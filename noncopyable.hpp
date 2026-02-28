#ifndef MYMUDUO_NONCOPYABLE_HPP
#define MYMUDUO_NONCOPYABLE_HPP
#pragma once


class noncopyable {
public:
  noncopyable() = default;
  ~noncopyable() = default;

protected:
  noncopyable(const noncopyable &) = delete;
  noncopyable &operator=(const noncopyable &) = delete;
};

// todo: namespace mymuduo

#endif // MYMUDUO_NONCOPYABLE_HPP