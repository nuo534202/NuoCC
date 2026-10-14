#include <stddef.h>

int zero(void);
void nothing(void);

int zero(void) { return(0); }

void nothing(void) {}

/* A void pointer stands beside the other parameters. */
int count(void *start, int limit) {
    return(limit);
}

char *hex_text= "\x41\x42\x43";
char *octal_text= "\101\102\103";
char nul= '\0';
char tab= '\t';
char escapes[4]= { '\0', '\101', '\x42', 0 };
int hex_number= 0x100;
int octal_number= 0100;
int mixed= 0x10 + 010;

int main() {
    print zero();
    nothing();
    print count(NULL, 0x1f);

    print hex_number;
    print octal_number;
    print mixed;
    print 0x7b;
    print 0173;
    print 123;

    print nul;
    print tab;
    print hex_text[0];
    print hex_text[2];
    print octal_text[1];
    print escapes[0];
    print escapes[1];
    print escapes[2];
    return(0);
}
