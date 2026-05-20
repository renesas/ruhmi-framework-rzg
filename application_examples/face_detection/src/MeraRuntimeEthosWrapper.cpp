/*
 * (C) Copyright EdgeCortix, Inc. 2024
 */

#include <iostream>
#include <fstream>
#include <numeric>
#include <string>
#include <regex>
#include <sstream>
#include <dirent.h>

#include "MeraRuntimeEthosWrapper.h"
#include "mera2_runtime_plan/plan_io.h"
#include "mera_runtime.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

using namespace mera2_runtime_plan;

template <typename T>
std::vector<T> LoadBinary(const std::string& bin_file) {
  std::ifstream file(bin_file.c_str(), std::ios::in | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("unable to open file " + bin_file);
  }

  file.seekg(0, file.end);
  const uint32_t file_size = static_cast<uint32_t>(file.tellg());
  file.seekg(0, file.beg);

  const auto file_buffer = std::unique_ptr<char>(new char[file_size]);
  file.read(file_buffer.get(), file_size);

  if (file.bad() || file.fail()) {
    throw std::runtime_error("error occured while reading the file");
  }

  file.close();

  auto ptr = reinterpret_cast<T*>(file_buffer.get());
  const auto num_elements = file_size / sizeof(T);
  return std::vector<T>(ptr, ptr + num_elements);
}

class MeraRuntimeEthosWrapper::Impl {
 public:
  Impl() {
    console_ = spdlog::get("console");
    if (!console_) {
      // The logger was not registered, so need do it.
      console_ = spdlog::stdout_color_mt("console");
    }
  }
  virtual ~Impl() {};
  virtual bool LoadModel(const std::string& model_dir) = 0;
  virtual void SetInput(int input_index, const float* data_ptr) = 0;
  virtual void SetInput(int input_index, const uint8_t* data_ptr) = 0;
  virtual void SetInput(int input_index, const int8_t* data_ptr) = 0;

  virtual std::vector<std::tuple<std::string, size_t, InOutDataType>> GetInputInfo() = 0;
  virtual void* GetInputPtr(const std::string& name) = 0;
  virtual std::vector<std::tuple<std::string, size_t, InOutDataType>> GetOutputInfo() = 0;
  virtual void* GetOutputPtr(const std::string& name) = 0;

  virtual void Run() = 0;
  virtual int GetNumInput(std::string model_dir) = 0;
  virtual InOutDataType GetInputDataType(int index) = 0;
  virtual int GetNumOutput() = 0;
  virtual std::tuple<InOutDataType, void*, int64_t> GetOutput(int index) = 0;
 protected:
  using Plan_DType_T = mera2_runtime_plan::Plan::MemoryPlan::DType;
  InOutDataType GetInOutDataType(const Plan_DType_T plan_dtype) {
    switch (plan_dtype) {
      case Plan_DType_T::FLOAT32: return InOutDataType::FLOAT32;
      case Plan_DType_T::UINT8:   return InOutDataType::UINT8;
      case Plan_DType_T::INT8:    return InOutDataType::INT8;
      case Plan_DType_T::INT64:   return InOutDataType::INT64;
      case Plan_DType_T::INT32:   return InOutDataType::INT32;
      default: throw std::runtime_error("Unsupported dtype");
    }
  }

  std::shared_ptr<spdlog::logger> console_;
};

class ImplMera2 : public MeraRuntimeEthosWrapper::Impl {
public:
  ImplMera2(): MeraRuntimeEthosWrapper::Impl() {}
  ~ImplMera2() = default;

  bool LoadModel(const std::string& model_dir) override {
    const std::string plan_file = model_dir + "/mera.plan";
    console_->info("MERA 2.0 Runtime");
    auto plan = mera2_runtime_plan::Mera2RuntimePlanLoad(plan_file);
    rt_ = std::make_unique<MeraRuntime>(plan, model_dir);
    rt_->Init();
    auto input_info = rt_->InputNames();
    auto output_info = rt_->OutputNames();
    int idx = 0;
    for (const auto& [name, buffer] : input_info) {
      index_to_input_buffers_[idx++] = buffer;
    }
    idx = 0;
    for (const auto& [name, buffer] : output_info) {
      index_to_output_buffers_[idx++] = buffer;
    }
    return true;
  }

  void SetInput(int input_index, const float* data_ptr) override {
    SetInputImpl(input_index, data_ptr);
  }
  void SetInput(int input_index, const int8_t* data_ptr) override {
    SetInputImpl(input_index, data_ptr);
  }
  void SetInput(int input_index, const uint8_t* data_ptr) override {
    SetInputImpl(input_index, data_ptr);
  }

  template <typename T>
  void SetInputImpl(int input_index, const T* data_ptr) {
    auto input_name = index_to_input_buffers_[input_index].name;
    auto size = index_to_input_buffers_[input_index].bytes_size;
    auto* input_ptr = reinterpret_cast<void*>(rt_->GetInputPtr(input_name));
    std::memcpy(input_ptr, data_ptr, size);
  }

  std::vector<std::tuple<std::string, size_t, InOutDataType>> GetInputInfo() {
    std::vector<std::tuple<std::string, size_t, InOutDataType>> inputs{};
    auto input_info = rt_->InputNames();

    for (const auto& [name, buffer] : input_info) {
      inputs.emplace_back(name, buffer.bytes_size, GetInOutDataType(buffer.data_type));
    }
    return std::move(inputs);
  }

  void* GetInputPtr(const std::string& name) {
    return reinterpret_cast<void*>(rt_->GetInputPtr(name));
  }

  std::vector<std::tuple<std::string, size_t, InOutDataType>> GetOutputInfo() {
    std::vector<std::tuple<std::string, size_t, InOutDataType>> outputs{};
    auto output_info = rt_->OutputNames();

    for (const auto& [name, buffer] : output_info) {
      outputs.emplace_back(name, buffer.bytes_size, GetInOutDataType(buffer.data_type));
    }
    return std::move(outputs);
  }

  void* GetOutputPtr(const std::string& name) {
    return reinterpret_cast<void*>(rt_->GetOutputPtr(name));
  }

  void Run() override {
    rt_->Run();
  }

  int GetNumInput(std::string model_dir) override {
    return index_to_input_buffers_.size();
  }

  InOutDataType GetInputDataType(int index) override {
    auto mera_dtype = index_to_input_buffers_[index].data_type;
    return GetInOutDataType(mera_dtype);
  }

  int GetNumOutput() override {
    return index_to_output_buffers_.size();
  }

  std::tuple<InOutDataType, void*, int64_t> GetOutput(int index) override {
    auto buffer = index_to_output_buffers_[index];
    auto* output_ptr = reinterpret_cast<void*>(rt_->GetOutputPtr(buffer.name));
    return std::make_tuple(GetInOutDataType(buffer.data_type), output_ptr, buffer.size);
  }

private:
  std::unordered_map<int, mera2_runtime_plan::Plan::MemoryPlan::Buffer> index_to_input_buffers_{};
  std::unordered_map<int, mera2_runtime_plan::Plan::MemoryPlan::Buffer> index_to_output_buffers_{};
  std::unique_ptr<MeraRuntime> rt_;
};

MeraRuntimeEthosWrapper::MeraRuntimeEthosWrapper() {};
MeraRuntimeEthosWrapper::~MeraRuntimeEthosWrapper() {};

bool MeraRuntimeEthosWrapper::LoadModel(const std::string& model_dir) {
  const std::string plan_file = model_dir + "/mera.plan";
  std::ifstream plan_fd(plan_file);
  if (plan_fd.good()) {
    impl_ = std::make_unique<ImplMera2>();
  } else {
    throw std::runtime_error("Mera plan not found");
  }
  return impl_->LoadModel(model_dir);
}

void MeraRuntimeEthosWrapper::SetInput(int input_index, const float* data_ptr) {
  impl_->SetInput(input_index, data_ptr);
}

void MeraRuntimeEthosWrapper::SetInput(int input_index, const uint8_t* data_ptr) {
  impl_->SetInput(input_index, data_ptr);
}

void MeraRuntimeEthosWrapper::SetInput(int input_index, const int8_t* data_ptr) {
  impl_->SetInput(input_index, data_ptr);
}

std::vector<std::tuple<std::string, size_t, InOutDataType>> MeraRuntimeEthosWrapper::GetInputInfo() {
  return impl_->GetInputInfo();
}

void* MeraRuntimeEthosWrapper::GetInputPtr(const std::string& name) {
  return impl_->GetInputPtr(name);
}

std::vector<std::tuple<std::string, size_t, InOutDataType>> MeraRuntimeEthosWrapper::GetOutputInfo() {
  return impl_->GetOutputInfo();
}

void* MeraRuntimeEthosWrapper::GetOutputPtr(const std::string& name) {
  return impl_->GetOutputPtr(name);
}

void MeraRuntimeEthosWrapper::Run() {
  impl_->Run();
}

int MeraRuntimeEthosWrapper::GetNumInput(std::string model_dir) {
  return impl_->GetNumInput(model_dir);
}

InOutDataType MeraRuntimeEthosWrapper::GetInputDataType(int index) {
  return impl_->GetInputDataType(index);
}

int MeraRuntimeEthosWrapper::GetNumOutput() {
  return impl_->GetNumOutput();
}

std::tuple<InOutDataType, void*, int64_t> MeraRuntimeEthosWrapper::GetOutput(int index) {
  return impl_->GetOutput(index);
}
