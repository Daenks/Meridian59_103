
#include <tchar.h>
#include <blakserv.h>

/////////////////////////////////////////////////////////////////////////////////////////////

#define OUTLOOPCOUNT  16
#define LOOPCOUNT     2048*2048
#define LOOPCOUNTBSP  2048*16

static LARGE_INTEGER frequency;
static LARGE_INTEGER start;
static LARGE_INTEGER end;
static double interval;
static V3 values[LOOPCOUNT];
static V3* allocvalues;
static float dotproducts[LOOPCOUNT];
static Wall* wall;
static room_type roomTos;

DECLSPEC_NOINLINE static const void UpdateInterval()
{
	interval = (double)(end.QuadPart - start.QuadPart) / (double)frequency.QuadPart;
}

/////////////////////////////////////////////////////////////////////////////////////////////

DECLSPEC_NOINLINE static const void LoadRooms()
{
	// load tos
	if (!BSPLoadRoom("../../resource/rooms/tos.roo", &roomTos))
		printf("Error loading file ../../resource/rooms/tos.roo\n");

	// tos blockers
	V2 bP1 = { 15000.0f, 15000.0f };
	BSPBlockerAdd(&roomTos, 1, &bP1);
	V2 bP2 = { 10000.0f, 15000.0f };
	BSPBlockerAdd(&roomTos, 2, &bP2);
	V2 bP3 = { 8000.0f, 4000.0f };
	BSPBlockerAdd(&roomTos, 3, &bP3);
	V2 bP4 = { 25000.0f, 25000.0f };
	BSPBlockerAdd(&roomTos, 4, &bP4);
}

/////////////////////////////////////////////////////////////////////////////////////////////

DECLSPEC_NOINLINE static const void GenerateRandoms()
{	
	// seed randomgen with current timestamp
	QueryPerformanceCounter(&start);
	srand(start.LowPart);

	for (size_t i = 0; i < LOOPCOUNT; i++)
	{
		values[i].X = allocvalues[i].X = (float)rand() * (1.0f / (float)RAND_MAX) * 10.0f;
		values[i].Y = allocvalues[i].Y = (float)rand() * (1.0f / (float)RAND_MAX) * 10.0f;
		values[i].Z = allocvalues[i].Z = (float)rand() * (1.0f / (float)RAND_MAX) * 10.0f;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

#pragma region BaseTests
DECLSPEC_NOINLINE static const void TestLoopEmpty()
{
	QueryPerformanceCounter(&start);
	__asm
	{
		mov ecx, OUTLOOPCOUNT

		outerloop:
			mov eax, LOOPCOUNT - 7

		innerloop:
			dec     eax
			jnz     innerloop

		dec ecx
		jnz outerloop
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("EMPTY LOOP   Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestMOVAPSFrom()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		__asm { MOVAPS xmm0, xmmword ptr values[0] }
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("MOVAPS FROM  Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestMOVAPSTo()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		__asm { MOVAPS xmmword ptr values[0], xmm0 }
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("MOVAPS TO    Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestMOVUPSFrom()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		__asm { MOVUPS xmm0, values[0] }
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("MOVUPS FROM  Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestMOVUPSTo()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		__asm { MOVUPS values[0], xmm0 }
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("MOVUPS TO    Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestMOVSSFrom()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		__asm { MOVSS xmm0, dword ptr values[0] }
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("MOVSS FROM   Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestMOVSSTo()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		__asm { MOVSS dword ptr values[0], xmm0 }
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("MOVSS TO     Time: %f\n", interval);
}
#pragma endregion

/////////////////////////////////////////////////////////////////////////////////////////////

#pragma region V3
DECLSPEC_NOINLINE static const void TestV3Add()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		V3ADD(&values[i - 1], &values[i - 5], &values[i - 3]);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("V3ADD        Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestV3Sub()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		V3SUB(&values[i - 1], &values[i - 5], &values[i - 3]);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("V3SUB        Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestV3Dot()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		dotproducts[i] = V3DOT(&values[i - 5], &values[i - 3]);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("V3DOT        Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestV3Cross()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		V3CROSS(&values[i - 1], &values[i - 5], &values[i - 3]);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("V3CROSS      Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestV3Scale()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		V3SCALE(&values[i-1], values[i].X);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("V3SCALE      Time: %f \n", interval);
}

DECLSPEC_NOINLINE static const void TestV3Len()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		dotproducts[i] = V3LEN(&values[i]);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("V3LEN        Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestV3Normalize()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		V3NORMALIZE(&values[i - 1]);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("V3NORMALIZE  Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestV3Round()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		V3ROUND(&values[i - 1]);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("V3ROUND      Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestV3IsZero()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
		for (size_t i = 7; i < LOOPCOUNT; i++)
		{
			*(int*)&dotproducts = V3ISZERO(&values[i - 6]);
		}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("V3ISZERO     Time: %f\n", interval);
}
#pragma endregion

/////////////////////////////////////////////////////////////////////////////////////////////

#pragma region V2
DECLSPEC_NOINLINE static const void TestV2Add()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		V2ADD(&values[i - 1], &values[i - 3], &values[i - 5]);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("V2ADD        Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestV2Sub()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		V2SUB(&values[i - 1], &values[i - 3], &values[i - 5]);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("V2SUB        Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestV2Scale()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		V2SCALE(&values[i - 1], values[i].X);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("V2SCALE      Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestV2Dot()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		dotproducts[i] = V2DOT(&values[i - 1], &values[i]);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("V2DOT        Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestV2Len()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		dotproducts[i] = V2LEN(&values[i]);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("V2LEN        Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestV2Round()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		V2ROUND(&values[i - 1]);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("V2ROUND      Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestV2IsZero()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		*(int*)&dotproducts = V2ISZERO(&values[i - 6]);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("V2ISZERO     Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestV2IsInBox()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		*(int*)&dotproducts[i] = ISINBOX(&values[i - 1], &values[i-3], &values[i-5]);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("V2ISINBOX    Time: %f\n", interval);
}
#pragma endregion

/////////////////////////////////////////////////////////////////////////////////////////////

DECLSPEC_NOINLINE static const void TestIntersectLineTriangle()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		dotproducts[i] = IntersectLineTriangle(&allocvalues[i - 7], &allocvalues[i - 6], &allocvalues[i - 5], &values[i - 4], &values[i - 3]);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("INT-LIN-TRIA Time: %f \n", interval);
}

DECLSPEC_NOINLINE static const void TestMinSquaredDistanceToLineSegment()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		dotproducts[i] = MinSquaredDistanceToLineSegment((V2*)&values[i - 7], (V2*)&allocvalues[i - 6], (V2*)&allocvalues[i - 5]);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("MIN-DIST-LIN Time: %f\n", interval);
}

DECLSPEC_NOINLINE static const void TestIntersectLineCircle()
{
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNT; i++)
	{
		dotproducts[i] = IntersectLineCircle((V2*)&values[i - 7], 2.0f, (V2*)&allocvalues[i - 6], (V2*)&allocvalues[i - 5]);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("INT-LIN-CIRC Time: %f\n", interval);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//                              BSP Benchmarks
/////////////////////////////////////////////////////////////////////////////////////////////
DECLSPEC_NOINLINE static const void TestBSPCanMoveInRoom()
{
	V2 s = { 16384.0f, 16384.0f };
	V2 e = { 16384.0f, 32768.0f };
	bool can;
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNTBSP; i++)
	{
		can = BSPCanMoveInRoom(&roomTos, &s, &e, 0, false, &wall);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("BSPCANMOVTOS Time: %f  CAN=%i\n", interval, can);
}

DECLSPEC_NOINLINE static const void TestBSPGetHeight()
{
	V3 s = { 16384.0f, 16384.0f, 0.0f };
	bool ins;
	float tmp;
	BspLeaf* leaf;
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNTBSP; i++)
	{
		ins = BSPGetHeight(&roomTos, (V2*)&s, &tmp, &s.Z, &tmp, &leaf);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("BSPGETHEIGHT Time: %f  INS=%i S.Z=%f\n", interval, ins, s.Z);
}

DECLSPEC_NOINLINE static const void TestBSPLineOfSight1()
{
	V3 s = { 21000.0f, 13832.0f, 0.0f }; // duke door
	V3 e = { 21000.0f, 63144.0f, 0.0f }; // close to fence at fountain
	float tmp;
	BspLeaf* leaf;
	bool ins, ine, los;
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNTBSP; i++)
	{
		ins = BSPGetHeight(&roomTos, (V2*)&s, &tmp, &s.Z, &tmp, &leaf);
		ine = BSPGetHeight(&roomTos, (V2*)&e, &tmp, &e.Z, &tmp, &leaf);
		los = BSPLineOfSight(&roomTos, &s, &e);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("BSPLOSTOS1   Time: %f  LOS=%i INS=%i INE=%i S.Z=%.2f E.Z=%.2f\n", interval, los, ins, ine, s.Z, e.Z);
}

DECLSPEC_NOINLINE static const void TestBSPLineOfSight2()
{
	V3 s = { 16384.0f, 16384.0f, 2623.0f };
	V3 e = { 24000.0f, 32768.0f, 3746.0f };
	float tmp;
	BspLeaf* leaf;
	bool ins, ine, los;
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNTBSP; i++)
	{
		ins = BSPGetHeight(&roomTos, (V2*)&s, &tmp, &s.Z, &tmp, &leaf);
		ine = BSPGetHeight(&roomTos, (V2*)&e, &tmp, &e.Z, &tmp, &leaf);
		los = BSPLineOfSight(&roomTos, &s, &e);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("BSPLOSTOS2   Time: %f  LOS=%i INS=%i INE=%i S.Z=%.2f E.Z=%.2f\n", interval, los, ins, ine, s.Z, e.Z);
}

DECLSPEC_NOINLINE static const void TestBSPLineOfSight3()
{
	V3 s = { 24000.0f, 32768.0f, 3746.0f };
	V3 e = { 16384.0f, 16384.0f, 2623.0f };
	float tmp;
	BspLeaf* leaf;
	bool ins, ine, los;
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNTBSP; i++)
	{
		ins = BSPGetHeight(&roomTos, (V2*)&s, &tmp, &s.Z, &tmp, &leaf);
		ine = BSPGetHeight(&roomTos, (V2*)&e, &tmp, &e.Z, &tmp, &leaf);
		los = BSPLineOfSight(&roomTos, &s, &e);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("BSPLOSTOS3   Time: %f  LOS=%i INS=%i INE=%i S.Z=%.2f E.Z=%.2f\n", interval, los, ins, ine, s.Z, e.Z);
}

DECLSPEC_NOINLINE static const void TestBSPGetStepTowards()
{
	V2 s = { 16384.0f, 16384.0f };
	V2 e = { 16384.0f, 32768.0f };
	V2 p;
	unsigned int flags;
	bool can;
	QueryPerformanceCounter(&start);
	for (size_t j = 0; j < OUTLOOPCOUNT; j++)
	for (size_t i = 7; i < LOOPCOUNTBSP; i++)
	{
		can = BSPGetStepTowards(&roomTos, &s, &e, &p, &flags, 0);
	}
	QueryPerformanceCounter(&end);
	UpdateInterval();
	printf("BSPGETSTEP   Time: %f  CAN=%i\n", interval, can);
}
/////////////////////////////////////////////////////////////////////////////////////////////

int _tmain(int argc, _TCHAR* argv[])
{
	char strLoopsBase[64];
	char strLoopsBsp[64];
	sprintf_s(strLoopsBase, " GEOMETRY BENCHMARK    LOOPS: %i\n", OUTLOOPCOUNT*LOOPCOUNT);
	sprintf_s(strLoopsBsp,  " ROOFILE BENCHMARK     LOOPS: %i\n", OUTLOOPCOUNT*LOOPCOUNTBSP);

	allocvalues = (V3*)_aligned_malloc(LOOPCOUNT * sizeof(V3), 16);
	
	QueryPerformanceFrequency(&frequency);
	LoadRooms();
	
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	
	while (1)
	{
		system("cls");
		
		////////////////////////////////////////////////////////////////////////////////////
		printf(strLoopsBase);
		printf("---------------------------------------------\n");
		////////////////////////////////////////////////////////////////////////////////////
		GenerateRandoms();
		TestLoopEmpty();
		/*GenerateRandoms();
		TestMOVAPSFrom();
		GenerateRandoms();
		TestMOVAPSTo();
		GenerateRandoms();
		TestMOVUPSFrom();
		GenerateRandoms();
		TestMOVUPSTo();
		GenerateRandoms();
		TestMOVSSFrom();
		GenerateRandoms();
		TestMOVSSTo();*/
		////////////////////////////////////////////////////////////////////////////////////
		GenerateRandoms();
		TestV3Add();
		GenerateRandoms();
		TestV3Sub();
		GenerateRandoms();
		TestV3Scale();
		GenerateRandoms();
		TestV3Dot();
		GenerateRandoms();
		TestV3Cross();
		GenerateRandoms();
		TestV3Len(); // invalid loopcount in sse2
		GenerateRandoms();
		TestV3Normalize();
		GenerateRandoms();
		TestV3Round();
		GenerateRandoms();
		TestV3IsZero();
		////////////////////////////////////////////////////////////////////////////////////
		GenerateRandoms();
		TestV2Add();
		GenerateRandoms();
		TestV2Sub();
		GenerateRandoms();
		TestV2Scale();
		//GenerateRandoms();
		//TestV2Dot(); // invalid loopcount in sse2
		GenerateRandoms();
		TestV2Len(); // invalid loopcount in sse2
		GenerateRandoms();
		TestV2Round();
		GenerateRandoms();
		TestV2IsZero();
		GenerateRandoms();
		TestV2IsInBox();
		////////////////////////////////////////////////////////////////////////////////////
		GenerateRandoms();
		TestIntersectLineTriangle();
		GenerateRandoms();
		TestIntersectLineCircle();
		GenerateRandoms();
		TestMinSquaredDistanceToLineSegment();
		////////////////////////////////////////////////////////////////////////////////////
		printf("---------------------------------------------\n");
		printf(strLoopsBsp);
		printf("---------------------------------------------\n");
		////////////////////////////////////////////////////////////////////////////////////	
		GenerateRandoms();
		TestBSPCanMoveInRoom();
		GenerateRandoms();
		TestBSPGetHeight();
		GenerateRandoms();
		TestBSPLineOfSight1();
		GenerateRandoms();
		TestBSPLineOfSight2();
		GenerateRandoms();
		TestBSPLineOfSight3();
		GenerateRandoms();
		TestBSPGetStepTowards();
		////////////////////////////////////////////////////////////////////////////////////
		printf("---------------------------------------------\n");
		printf("          PRESS KEY TO RESTART \n");
		getchar();
	}

	return 0;
}

