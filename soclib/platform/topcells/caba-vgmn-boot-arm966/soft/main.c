unsigned int test_parameter() {
	int ret;

	ret = 34;

	return ret;
}

void c_start() {
  int a;
  int b;
  int c;

  a = test_parameter();
  b = 3;
  c = a + b;
}

