#include "rpcChannel.h"
#include "rpcHeader.pb.h"
#include "serviceDiscovery.h"
#include <arpa/inet.h>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// 用于一致性哈希算法的req_id
static std::atomic<int> g_req_id{0};

static ssize_t send_all(int fd, const char *buf, size_t len) {
  size_t sent = 0;
  while (sent < len) {
    ssize_t n = ::send(fd, buf + sent, len - sent, MSG_NOSIGNAL);
    if (n == -1) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    if (n == 0)
      return -1;
    sent += static_cast<size_t>(n);
  }
  return static_cast<ssize_t>(sent);
}

ssize_t RpcChannel::recv_exact(int fd, char *buf, size_t len) {
  size_t got = 0;
  while (got < len) {
    ssize_t n = ::recv(fd, buf + got, len - got, 0);
    if (n == -1) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    if (n == 0)
      return static_cast<ssize_t>(got); // 对端关闭
    got += static_cast<size_t>(n);
  }
  return static_cast<ssize_t>(got);
}

void RpcChannel::CallMethod(const ::google::protobuf::MethodDescriptor *method,
                            ::google::protobuf::RpcController *controller,
                            const ::google::protobuf::Message *request,
                            ::google::protobuf::Message *response,
                            ::google::protobuf::Closure *done) {
  std::string service_name(method->service()->name());
  std::string method_name(method->name());

  static std::once_flag init_flag;
  std::call_once(init_flag, []() { ServiceDiscovery::GetInstance().Init(); });

  // 原子取原值，然后+1
  uint64_t req_id = g_req_id.fetch_add(1, std::memory_order_relaxed);
  std::string route_key = std::to_string(req_id);

  std::string ip_port =
      ServiceDiscovery::GetInstance().GetTargetNode(service_name, route_key);
  if (ip_port.empty()) {
    controller->SetFailed("Hash ring returned empty node");
    return;
  }
  size_t pos = ip_port.find(":");
  std::string target_ip = ip_port.substr(0, pos);
  uint16_t target_port = atoi(ip_port.substr(pos + 1).c_str());

  // ---------- 1) 直接建连（替换连接池） ----------
  int clientfd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (clientfd == -1) {
    controller->SetFailed("Create socket failed!");
    return;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(target_port);
  if (::inet_pton(AF_INET, target_ip.c_str(), &addr.sin_addr) != 1) {
    ::close(clientfd);
    controller->SetFailed("Invalid server ip!");
    return;
  }

  if (::connect(clientfd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) ==
      -1) {
    ::close(clientfd);
    controller->SetFailed(std::string("Connect server failed: ") +
                          std::strerror(errno));
    return;
  }

  // ---------- 2) 序列化 + 发送 + 接收 + 解析 ----------
  // 出错时统一 close 后返回，用一个 lambda 简化清理
  auto fail_and_close = [&](const std::string &msg) {
    controller->SetFailed(msg);
    ::close(clientfd);
  };

  std::string args_str;
  if (!request->SerializeToString(&args_str)) {
    fail_and_close("Serialize request fail");
    return;
  }

  RPC::RpcHeader Rpcheader;
  Rpcheader.set_service_name(service_name);
  Rpcheader.set_method_name(method_name);
  Rpcheader.set_args_size(args_str.size());

  std::string rpc_header_str;
  Rpcheader.SerializeToString(&rpc_header_str);

  uint32_t header_size = static_cast<uint32_t>(rpc_header_str.size());
  uint32_t net_header_len = htonl(header_size);

  // 新协议: | header_len(4B) | RPCHeader | Arg_string |
  std::string send_rpc_str;
  send_rpc_str.reserve(4 + header_size + args_str.size());
  send_rpc_str.append(reinterpret_cast<char *>(&net_header_len), 4);
  send_rpc_str.append(rpc_header_str);
  send_rpc_str.append(args_str);

  if (send_all(clientfd, send_rpc_str.data(), send_rpc_str.size()) == -1) {
    fail_and_close("Send rpc request error!");
    return;
  }

  uint32_t response_len = 0;
  if (recv_exact(clientfd, reinterpret_cast<char *>(&response_len), 4) != 4) {
    fail_and_close("Recv response header error!");
    return;
  }
  response_len = ntohl(response_len);

  std::vector<char> recv_buf(response_len);
  if (recv_exact(clientfd, recv_buf.data(), response_len) !=
      static_cast<ssize_t>(response_len)) {
    fail_and_close("Recv response body error!");
    return;
  }

  if (!response->ParseFromArray(recv_buf.data(), response_len)) {
    fail_and_close("Parse response error!");
    return;
  }

  // ---------- 3) 关闭连接（替换 ReturnConnection） ----------
  ::close(clientfd);
}