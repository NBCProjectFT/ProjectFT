#include "FTStaffCharacter.h"

AFTStaffCharacter::AFTStaffCharacter()
{
	// 캐셔는 제자리 고정 NPC라 이동속도가 0이지만, 직원은 매대까지 이동해야 한다.
	InitialMoveSpeed = 200.0f;
}
