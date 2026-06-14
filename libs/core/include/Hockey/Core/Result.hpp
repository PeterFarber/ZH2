#pragma once
#include <string>
#include <utility>
namespace Hockey {
struct Status { bool success=false; std::string error; static Status Ok(){return {true,{}};} static Status Fail(std::string msg){return {false,std::move(msg)};} explicit operator bool() const { return success; } };
template<typename T> struct Result { bool success=false; T value{}; std::string error; static Result<T> Ok(T v){ Result<T> r; r.success=true; r.value=std::move(v); return r; } static Result<T> Fail(std::string msg){ Result<T> r; r.success=false; r.error=std::move(msg); return r; } explicit operator bool() const { return success; } };
}
