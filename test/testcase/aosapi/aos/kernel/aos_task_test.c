/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>

#include <k_api.h>
#include <yos/kernel.h>

#include <yunit.h>

static yos_sem_t sync_sem;
#define TEST_TASK_STACK_SIZE (8192)

////////////////////////////////////////////////////////////////////////////////////////////////
void TASK_aosapi_kernel_task_new_param(void *arg)
{
	yos_task_exit(0);
}
static void CASE_aosapi_kernel_task_new_param()
{
	int ret = RHINO_SUCCESS;
	ret = yos_task_new(NULL, TASK_aosapi_kernel_task_new_param, NULL, 1024);
	YUNIT_ASSERT_MSG(ret==RHINO_NULL_PTR, "ret=%d", ret);

	ret = yos_task_new("TASK_aosapi_kernel_task_new_param", NULL, NULL, 1024);
	YUNIT_ASSERT_MSG(ret==RHINO_NULL_PTR, "ret=%d", ret);

#if 1
	ret = yos_task_new("TASK_aosapi_kernel_task_new_param",
			           TASK_aosapi_kernel_task_new_param, NULL, 0);
	YUNIT_ASSERT_MSG(ret==RHINO_INV_PARAM, "ret=%d", ret);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////
void TASK_aosapi_kernel_task_new_batch(void *arg)
{
	yos_sem_signal(&sync_sem);
	yos_task_exit(0);
}
static void CASE_aosapi_kernel_task_new_batch()
{
	int i = 0;
	int success_count = 0;
	int ret = RHINO_SUCCESS;
	const int TASK_COUNT = 100;
	for(i=0; i<TASK_COUNT; i++) {
		ret = yos_sem_new(&sync_sem, 0);
		YUNIT_ASSERT_MSG(ret==RHINO_SUCCESS, "ret=%d", ret);

		ret = yos_task_new("TASK_aosapi_kernel_task_new_batch",
						   TASK_aosapi_kernel_task_new_batch, NULL, TEST_TASK_STACK_SIZE);
		YUNIT_ASSERT_MSG(ret==RHINO_SUCCESS, "ret=%d", ret);
		if(ret != RHINO_SUCCESS) {
			yos_sem_signal(&sync_sem);
		}
		ret = yos_sem_wait(&sync_sem, RHINO_WAIT_FOREVER);
		YUNIT_ASSERT_MSG(ret==RHINO_SUCCESS, "ret=%d", ret);

		yos_sem_free(&sync_sem);
		success_count += (ret==RHINO_SUCCESS ? 1 : 0);
		printf("task %d\t", success_count);
	}
	YUNIT_ASSERT(success_count == TASK_COUNT);
}

////////////////////////////////////////////////////////////////////////////////////////////////
void TASK_aosapi_kernel_task_new_stack(void *arg)
{
	int array[2048];
    memset(array, 0, sizeof(int)*2048);
	PRINT_TASK_INFO(krhino_cur_task_get());
	yos_task_exit(0);
}
static void CASE_aosapi_kernel_task_new_stack()
{
#if 0
	int ret = RHINO_SUCCESS;
	ret = yos_task_new("TASK_aosapi_kernel_task_new_stack",
					   TASK_aosapi_kernel_task_new_stack, NULL, 256);
	YUNIT_ASSERT_MSG(ret==RHINO_SUCCESS, "ret=%d", ret);
	yos_msleep(1000);
#endif

#if 0
	ret = yos_task_new("TASK_aosapi_kernel_task_new_stack",
			           TASK_aosapi_kernel_task_new_stack, NULL, 10);
	YUNIT_ASSERT_MSG(ret==RHINO_SUCCESS, "ret=%d", ret);
	yos_msleep(1000);
#endif

#if 0
	ret = yos_task_new("TASK_aosapi_kernel_task_new_stack",
			           TASK_aosapi_kernel_task_new_stack, NULL, 20480000);
	YUNIT_ASSERT_MSG(ret==RHINO_SUCCESS, "ret=%d", ret);
	yos_msleep(1000);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////
void TASK_aosapi_kernel_task_getname(void *arg)
{
	YUNIT_ASSERT(0==strcmp(yos_task_name(), "TASK_aosapi_kernel_task_getname"));
	yos_sem_signal(&sync_sem);
	yos_task_exit(0);
}
static void CASE_aosapi_kernel_task_getname()
{
	int ret = RHINO_SUCCESS;
	ret= yos_sem_new(&sync_sem, 0);
	YUNIT_ASSERT_MSG(ret==RHINO_SUCCESS, "ret=%d", ret);

	ret = yos_task_new("TASK_aosapi_kernel_task_getname",
			           TASK_aosapi_kernel_task_getname, NULL, TEST_TASK_STACK_SIZE);
	YUNIT_ASSERT_MSG(ret==RHINO_SUCCESS, "ret=%d", ret);
	if(ret != RHINO_SUCCESS) {
		yos_sem_signal(&sync_sem);
	}

	ret = yos_sem_wait(&sync_sem, RHINO_WAIT_FOREVER);
	YUNIT_ASSERT_MSG(ret==RHINO_SUCCESS, "ret=%d", ret);

	yos_sem_free(&sync_sem);
}

////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
void TASK_aosapi_kernel_task_new_priority(void *arg)
{
	yos_sem_signal(&sync_sem);
	yos_task_exit(0);
}
static void CASE_aosapi_kernel_task_new_priority()
{
	int ret = RHINO_SUCCESS;
	int success_count = 0;
	int i = 0;

#if 1
	/*  */
	ret = yos_sem_new(&sync_sem, 0);
	YUNIT_ASSERT_MSG(ret==RHINO_SUCCESS, "ret=%d", ret);

	ret = yos_task_new_ext("TASK_aosapi_kernel_task_new_priority",
			               TASK_aosapi_kernel_task_new_priority, NULL,
			               TEST_TASK_STACK_SIZE, RHINO_CONFIG_PRI_MAX);
	YUNIT_ASSERT_MSG(ret==RHINO_BEYOND_MAX_PRI, "ret=%d", ret);
	if(ret == RHINO_BEYOND_MAX_PRI) {
		yos_sem_signal(&sync_sem);
	}
	ret = yos_sem_wait(&sync_sem, RHINO_WAIT_FOREVER);
	YUNIT_ASSERT_MSG(ret==RHINO_SUCCESS, "ret=%d", ret);
	yos_sem_free(&sync_sem);

	/*  */
	ret = yos_sem_new(&sync_sem, 0);
	YUNIT_ASSERT_MSG(ret==RHINO_SUCCESS, "ret=%d", ret);

	ret = yos_task_new_ext("TASK_aosapi_kernel_task_new_priority",
			               TASK_aosapi_kernel_task_new_priority, NULL,
						   TEST_TASK_STACK_SIZE, RHINO_CONFIG_USER_PRI_MAX);
	YUNIT_ASSERT_MSG(ret==RHINO_SUCCESS, "ret=%d", ret);
	if(ret != RHINO_SUCCESS) {
		yos_sem_signal(&sync_sem);
	}
	ret = yos_sem_wait(&sync_sem, RHINO_WAIT_FOREVER);
	YUNIT_ASSERT_MSG(ret==RHINO_SUCCESS, "ret=%d", ret);
	yos_sem_free(&sync_sem);
#endif

	/*  */
#if 1
	int pris[] = {11,12,13};
	for(i=0; i<sizeof(pris)/sizeof(pris[0]); i++) {
		ret = yos_sem_new(&sync_sem, 0);
		YUNIT_ASSERT_MSG(ret==RHINO_SUCCESS, "ret=%d", ret);

		ret = yos_task_new_ext("TASK_aosapi_kernel_task_new_priority",
				           TASK_aosapi_kernel_task_new_priority, NULL,
						   TEST_TASK_STACK_SIZE, pris[i]);
		YUNIT_ASSERT_MSG(ret==RHINO_SUCCESS, "ret=%d", ret);
		if(ret != RHINO_SUCCESS) {
			yos_sem_signal(&sync_sem);
		}
		ret = yos_sem_wait(&sync_sem, RHINO_WAIT_FOREVER);
		YUNIT_ASSERT_MSG(ret==RHINO_SUCCESS, "ret=%d", ret);

		yos_sem_free(&sync_sem);
		success_count += (ret==RHINO_SUCCESS ? 1 : 0);
		printf("\ttask prio=%d\n", pris[i]);
	}
	YUNIT_ASSERT_MSG(success_count==sizeof(pris)/sizeof(pris[0]), "success_count=%d", success_count);
#endif
}
#endif


void aosapi_kernel_task_test_entry(yunit_test_suite_t *suite)
{
	yunit_add_test_case(suite, "kernel.task.param", CASE_aosapi_kernel_task_new_param);
	yunit_add_test_case(suite, "kernel.task.stack", CASE_aosapi_kernel_task_new_stack);
	yunit_add_test_case(suite, "kernel.task.batchcreate", CASE_aosapi_kernel_task_new_batch);
//	yunit_add_test_case(suite, "kernel.task.priority", CASE_aosapi_kernel_task_new_priority);
	yunit_add_test_case(suite, "kernel.task.getname", CASE_aosapi_kernel_task_getname);
}

