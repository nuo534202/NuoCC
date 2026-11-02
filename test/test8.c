/* This fixture intentionally uses NuoCC's documented print statement. */
int main(void)
{
  int i; int j;
  i= 6; j= 12;
  if (i < j) {
    print i;
  } else {
    print j;
  }
  return(0);
}
