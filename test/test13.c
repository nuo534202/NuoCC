int fred(int ignored) {
  return(56);
}

int main(void) {
  int result;
  printint(23);
  result= fred(10);
  printint(result);
    return(0);
}
