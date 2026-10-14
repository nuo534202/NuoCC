int x= 2;
char y= 'a';
char *str= "Hello";
long big= 100000;
int a[10];
char b[]= { 'q', 'w', 'e' };
char c[5]= { 'x', 'y' };
char *d[]= { "apple", "banana", "peach" };
int e[4]= { 1, 4, 17 };
int f[]= { 9, 8, 7, 6 };
int zero;
char *empty= "";

int main() {
    print x;
    print y;
    print str[0];
    print str[4];
    print a[0];
    print a[9];
    print b[0] + b[1] + b[2];
    print c[0];
    print c[1];
    print c[2];
    print c[4];
    print d[0][0];
    print d[2][1];
    print e[0] + e[1] + e[2] + e[3];
    print f[0] + f[3];
    print zero;

    if (big == 100000) { print 1; }
    print empty[0];
    return(0);
}
