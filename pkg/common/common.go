package common

import (
  "log"
  "sync"
)

type SafeInteger struct {
  value int
  initial int
  lock sync.Mutex
}


func NewSafeInteger(val int, init int) *SafeInteger {
  return &SafeInteger{
    value: val,
    initial: init,
    lock: sync.Mutex{},
  }
}

func (s *SafeInteger) Set(val int) {
  s.lock.Lock()
  defer s.lock.Unlock()
  log.Println("setting the value of safe integer ", val)
  s.value = val
}

func (s *SafeInteger) Get() int {
  s.lock.Lock()
  defer s.lock.Unlock()
  log.Println("getting the value of safe integer ", s.value)
  return s.value
}

func (s *SafeInteger) Boolean() bool {
  s.lock.Lock()
  defer s.lock.Unlock()
  if s.value > s.initial {
    return true
  }
  return false
}

func (s *SafeInteger) GetReset() int {
  s.lock.Lock()
  defer s.lock.Unlock()
  value := s.value
  if value != s.initial {
    log.Println("changing the value of safe integer ", s.initial, value)
    s.value = s.initial
  }
  return value
}
