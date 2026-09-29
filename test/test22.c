int g[5];
char c[4];
long l[3];
int p[10];

int main() {
  int i;
  int *q;
  int r;
  int a;
  char b[3];
  int z;
  int local[6];
  int *w;

  g[0]= 1;
  g[4]= 5;
  print g[0];
  print g[4];
  print g[2];

  for (i= 0; i < 5; i= i + 1) {
    g[i]= g[i] + i * 10;
  }
  print g[0];
  print g[1];
  print g[4];
  print g[i - 1];

  c[2]= 90;
  print c[2];
  c[3]= c[2] + 1;
  print c[3];

  l[1]= 100000;
  if (l[1] == 100000) { print 100000; }

  a= 1; b[0]= 2; b[1]= 3; b[2]= 4; z= 5;
  print a;
  print b[0];
  print b[1];
  print b[2];
  print z;

  for (i= 0; i < 6; i= i + 1) {
    local[i]= i + 100;
  }
  print local[0];
  print local[5];
  local[3]= local[3] + 5;
  print local[3];
  print local[3] - local[0];

  q= g;
  q[2]= 7;
  print g[2];
  print q[2];
  q= &g[0];
  print *q;
  q= &g[4];
  print *q;
  print *(g + 2);
  print *(q - 3);

  w= local;
  print w[1];
  w= &local[5];
  print *w;

  r= (g[1] + g[4]) * (g[0] + 1);
  print r;
  print (2 + 3) * (4 + 1);
  print p[9];
  print p[0];

  for (i= 0; i < 10; i= i + 1) {
    p[i]= i * i;
  }
  print p[9];
  print p[5];
  print p[i - 10];

  return(0);
}
