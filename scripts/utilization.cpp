#include <iostream>
#include <cmath>
#include <stdint.h>

uint64_t combination (uint64_t N, uint64_t k) {
  if (k > N) {
    return 0;
  }
  uint64_t comb = 1;
  for (uint64_t d = 1; d <= k; d++) {
    comb *= N;
    N--;
    comb /= d;
  }
  return comb;
}

int main (int argc, char** argv) {
  for (uint64_t B = 8; B <= 8; B+=8) {
    for (uint64_t N = 1; N <= 8; N++) {
      double utilization = 0;
      for (uint64_t b = 1; b <= B; b++) {
        std::cout<<pow(((double) b / (double) B), N+1) * combination(B, b)<<std::endl;
        utilization += pow(((double) b / (double) B), N+1) * combination(B, b);
      }
      std::cout<<B<<'\t'<<N<<'\t'<<utilization<<std::endl;
    }
  }
}