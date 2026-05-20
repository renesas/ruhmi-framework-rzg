/*
 * (C) Copyright EdgeCortix, Inc. 2024
 */

#ifndef MERA_DRP_RUNTIME_API_H_
#define MERA_DRP_RUNTIME_API_H_

#include <string>
#include <memory>
#include <ostream>

enum class InOutDataType {
  FLOAT32,
  UINT8,
  INT8,
  INT32,
  INT64,
  OTHER
};

class MeraRuntimeEthosWrapper {
public:
  MeraRuntimeEthosWrapper();
  ~MeraRuntimeEthosWrapper();

  bool LoadModel(const std::string& model_dir);

  void SetInput(int input_index, const float* data_ptr);
  void SetInput(int input_index, const int8_t* data_ptr);
  void SetInput(int input_index, const uint8_t* data_ptr);
  int GetNumInput(std::string model_dir);
  InOutDataType GetInputDataType(int index);
  int GetNumOutput();
  std::tuple<InOutDataType, void*, int64_t> GetOutput(int index);

  // Zero copy api.
  std::vector<std::tuple<std::string, size_t, InOutDataType>> GetInputInfo();
  void* GetInputPtr(const std::string& name);
  std::vector<std::tuple<std::string, size_t, InOutDataType>> GetOutputInfo();
  void* GetOutputPtr(const std::string& name);

  void Run();

  class Impl;
private:
  std::unique_ptr<Impl> impl_;
};

#endif /* MERA_DRP_RUNTIME_API_H_ */
