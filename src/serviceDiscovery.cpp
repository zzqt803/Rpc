#include "serviceDiscovery.h"
#include "consistentHash.h"
#include <shared_mutex>

ServiceDiscovery &ServiceDiscovery::GetInstance() {
  static ServiceDiscovery inst;
  return inst;
}

void ServiceDiscovery::Init() { zkClient_.Start(); }

std::string ServiceDiscovery::GetTargetNode(const std::string &service_name,
                                            const std::string &key) {
  // 进行一致性hash查询
  {
    std::shared_lock<std::shared_timed_mutex> read_lock(rw_mtx_);
    if (chash_map_.find(service_name) != chash_map_.end()) {
      return chash_map_[service_name]->GetTargetNode(key);
    }
  }

  // 如果hash里没有该服务,去zk拉取
  std::string path = "/" + service_name;
  std::vector<std::string> new_nodes =
      zkClient_.GetChildren(path, WatcherCallback, this);

  {
    std::unique_lock<std::shared_timed_mutex> write_lock;
    //再检查一次，防止其他线程已经初始化
    if (chash_map_.find(service_name) == chash_map_.end()) {
      chash_map_[service_name] =
          std::unique_ptr<ConsistentHash>(new ConsistentHash());
      chash_map_[service_name]->UpdateNodes(new_nodes);
      nodes_cache_[service_name] = new_nodes;
    }
    return chash_map_[service_name]->GetTargetNode(key);
  }
}

void ServiceDiscovery::WatcherCallback(zhandle_t *zh, int type, int state,
                                       const char *path, void *watcherCtx) {
  if (type == ZOO_CHILD_EVENT) {
    ServiceDiscovery *sd = static_cast<ServiceDiscovery *>(watcherCtx);
    std::string path_str = path;
    std::string service_name = path_str.substr(1);

    // 千万不要在这里加锁！先发起阻塞网络请求获取最新节点
    std::vector<std::string> new_nodes =
        sd->zkClient_.GetChildren(path_str.c_str(), WatcherCallback, sd);

    // 拿到数据后，再加写锁更新内存结构
    std::unique_lock<std::shared_timed_mutex> lock(sd->rw_mtx_);
    if (sd->chash_map_.find(service_name) != sd->chash_map_.end()) {
      sd->chash_map_[service_name]->UpdateNodes(new_nodes);
    }
    sd->nodes_cache_[service_name] = new_nodes;
  }
                                       }