#include "unity.h"

void run_bridge_state_tests(void);
void run_bridge_serial_tests(void);
void run_bridge_monitors_tests(void);
void run_bridge_ci_tests(void);

void setUp(void)
{
}
void tearDown(void)
{
}

int main(void)
{
	UNITY_BEGIN();
	run_bridge_state_tests();
	run_bridge_serial_tests();
	run_bridge_monitors_tests();
	run_bridge_ci_tests();
	return UNITY_END();
}
