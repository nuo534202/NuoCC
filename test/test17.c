int   c[2];
int  *e;
int   f;

int main() {
  c[0]= 12; c[1]= 18; printint(c[0]);
  e= &c[0] + 1; f= *e; printint(f);
  return(0);
}
