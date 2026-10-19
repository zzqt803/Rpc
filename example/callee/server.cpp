
#include "../user.pb.h"
#include "rpcApplication.h"
#include "rpcProvider.h"
#include <chrono>
#include <string>
#include <thread>

class UserService : public User::UserServiceRpc {
public:
  void Login(::google::protobuf::RpcController *controller,
             const ::User::LoginRequest *request,
             ::User::LoginResponse *response,
             ::google::protobuf::Closure *done) override {
    std::string name = request->name();
    std::string pwd = request->pwd();
    // 调用本地业务代码
    bool res = Login(name, pwd);
    // 使用可变参数，因为errmsg是可变的
    User::ResultCode *code = response->mutable_result();
    code->set_errcode(0);
    code->set_errmsg("");
    response->set_success(res);

    done->Run();
  }

  // 本地业务代码
  bool Login(std::string name, std::string pwd) {
    // 等待5毫秒，模拟数据库查询
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ;
    return true;
    ;
  }
};

int main(int argc, char *argv[]) {
  RpcApplication::Init(argc, argv);
  RpcProvider provider;
  provider.NotifyService(new UserService());
  provider.Run();
  return 0;
}