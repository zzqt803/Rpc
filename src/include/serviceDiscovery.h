#pragma once

#include "consistentHash.h"
#include "zookeeperUtil.h"
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <zookeeper/zookeeper.h>
class ServiceDiscovery {
public:
  static ServiceDiscovery &GetInstance();
  void Init();
  std::string GetTargetNode(const std::string &service_name,
                            const std::string &key);

private:
  ServiceDiscovery() = default;
  ~ServiceDiscovery() = default;
  ServiceDiscovery(const ServiceDiscovery &) = delete;
  ServiceDiscovery &operator=(const ServiceDiscovery &) = delete;

  ZkClient zkClient_;
  //每个服务都有一个一致性hash
  std::unordered_map<std::string, std::unique_ptr<ConsistentHash>> chash_map_;
  std::unordered_map<std::string, std::vector<std::string>> nodes_cache_;

  std::shared_timed_mutex rw_mtx_;

  //回调函数，当zk数据变化时，zk会调用该函数
  static void WatcherCallback(zhandle_t *zh, int type, int state,
                               const char *path, void *watherCtx);
  void UpdateCacheLocked(const std::string &service_name,
                         const std::string &path);
};