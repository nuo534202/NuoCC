/* This fixture intentionally uses NuoCC's documented print statement. */
int main(void)
{
  int i;
  for (i= 1; i <= 10; i= i + 1) {
    print i;
  }
  return(0);
}
