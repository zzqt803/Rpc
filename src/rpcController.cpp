
#include "rpcController.h"
RpcController::RpcController() {
  failed_ = false;
  error_text_ = "";
}

void RpcController::Reset() {
  failed_ = false;
  error_text_ = "";
}

bool RpcController::Failed() const { return failed_; }

std::string RpcController::ErrorText() const { return error_text_; }

void RpcController::SetFailed(const std::string &reason) {
  failed_ = true;
  error_text_ = reason;
}

// 以下功能未实现，是RPC服务端提供的取消功能
// 开始取消RPC调用（未实现）
void RpcController::StartCancel() {
  // 目前为空，未实现具体功能
}

// 判断RPC调用是否被取消（未实现）
bool RpcController::IsCanceled() const {
  return false; // 默认返回false，表示未被取消
}

// 注册取消回调函数（未实现）
void RpcController::NotifyOnCancel(google::protobuf::Closure *callback) {
  // 目前为空，未实现具体功能
}