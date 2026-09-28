int first() {
  int value;
  value = 41;
  return(value + 1);
}

int second() {
  int value;
  char byte;
  int *pointer;
  value = first();
  byte = 7;
  pointer = &value;
  print *pointer;
  return(value + byte);
}

int main() {
  print second();
  return(0);
}
