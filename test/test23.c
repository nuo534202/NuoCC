char c;
char *str;
char *p;

int main() {
  c = '\t';
  printint(c);
  c = '\n';
  printint(c);
  printint('Z');
  printchar('Z');
  printchar('\n');

  str = "Hello world\n";
  printint(str[0]);
  printint(*(str + 1));
  for (p = "Hello world\n"; *p != 0; p = p + 1) {
    printchar(*p);
  }

  for (p = "She said \"hi\"\t!\n"; *p != 0; p = p + 1) {
    printchar(*p);
  }

  str = "";
  printint(*str);
  printchar(c);

  return(0);
}
