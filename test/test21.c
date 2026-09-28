char a; char b; char c;
int  d; int  e; int  f;
long g; long h; long i;
int  x;

int main() {
  b= 5; c= 7; a= b + c++; print a;
  e= 5; f= 7; d= e + f++; print d;
  a= b-- + c; print a;
  d= e-- + f; print d;
  a= ++b + c; print a;
  d= ++e + f; print d;
  a= b * --c; print a;
  d= e * --f; print d;

  h= 5; i= 7; g= h + i++;
  if (g == 12) { print 12; }
  g= h-- + i;
  if (g == 13) { print 13; }
  if (h == 4) { print 4; }
  g= ++h + i;
  if (g == 13) { print 13; }
  if (h == 5) { print 5; }
  g= h * --i;
  if (g == 35) { print 35; }
  if (i == 7) { print 7; }

  x= -23; print x;
  print -10 * -10;
  x= 1; x= ~x; print x;
  x= 2 > 5; print x;
  x= !x; print x;
  x= !x; print x;
  x= 13; if (x) { print 13; }
  x= 0; if (!x) { print 14; }
  x= 5; while (x) { x--; }
  print x;

  print 42 & 19;
  print 42 | 19;
  print 42 ^ 19;
  print 1 << 3;
  print 63 >> 3;
  return(0);
}
