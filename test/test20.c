int a;
int b;
int c;
int *to_a;
int *to_b;

int main() {
  int local;
  int *to_local;
  char small;
  char *to_small;

  to_a= &a;
  *to_a= 12;
  print a;

  to_b= &b;
  *to_b= 30;
  *to_a= *to_b + 5;
  print a;

  c= a= 7;
  print a;
  print c;

  to_local= &local;
  *to_local= 99;
  print local;

  small= 1;
  to_small= &small;
  *to_small= 66;
  print small;

  return(0);
}
