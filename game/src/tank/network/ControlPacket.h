// ControlPacket.h

#pragma once

// forward declarations
struct VehicleState
{
	union
	{
		struct
		{
			bool _bState_RotateLeft;
			bool _bState_RotateRight;
		};
		float _fBodyAngle;
	};
	union
	{
		struct
		{
			bool _bState_TowerLeft;
			bool _bState_TowerRight;
			bool _bState_TowerCenter;
		};
		float _fTowerAngle;
	};
	union
	{
		struct
		{
			bool _bState_MoveForward : 1;
			bool _bState_MoveBack    : 1;
			bool _bExplicitBody      : 1;
			bool _bState_Fire        : 1;
			bool _bState_AllowDrop   : 1;
			bool _bLight             : 1;
			bool _bExplicitTower     : 1;
			bool _reserved           : 1;
		};
		BYTE flags;
	};
};

///////////////////////////////////////////////////////////////////////////////

#define STATE_MOVEFORWARD   0x0001
#define STATE_MOVEBACK      0x0002
#define STATE_ROTATELEFT    0x0004
#define STATE_ROTATERIGHT   0x0008
#define STATE_FIRE          0x0010
#define STATE_ALLOWDROP     0x0020
#define STATE_TOWERLEFT     0x0040
#define STATE_TOWERRIGHT    0x0080
#define STATE_TOWERCENTER   0x0100
#define STATE_ENABLELIGHT   0x0200

#define MODE_EXPLICITTOWER  0x4000
#define MODE_EXPLICITBODY   0x8000

#pragma pack(push)
#pragma pack(2)
class ControlPacket
{
public:
	WORD  wControlState;
	unsigned short weap;  // angle, if explicit
	unsigned short body;  // angle, if explicit
#ifdef NETWORK_DEBUG
	DWORD checksum;
#endif
	//--------------------------
	ControlPacket();
	//--------------------------
	void fromvs(const VehicleState &vs);
	void tovs(VehicleState &vs) const;
};
#pragma pack(pop)


// end of file