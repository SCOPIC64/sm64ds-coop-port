/* HOST COPY of _ZN3Boo8BehaviorEv (ov063 0x0211b0a4, 0x7e4 bytes), the Boo's
 * per-frame state spine -- vtable slot 6 of _ZTV3Boo, shared by Boo (209) and
 * BigBoo (210). NOT matched in src/ (no TU of this name exists on cons or
 * main as of 2026-08-13).
 *
 * PROVENANCE, so the next reader can re-derive the trust: this text is the
 * near-miss DB draft for 0x0211b0a4 with ONE repair (the ClosestPlayer
 * position copy rewritten from load-all-then-store-all to the interleaved
 * member copy). Verified against the ROM bytes with tools/abverify.py on
 * this worktree: EQUAL SIZE (505/505 words) and 45 divergences, every one a
 * register rename (r1/r3, r0/r2, ip/r3) on the same operation at the same
 * offset -- no missing or extra instruction, all 52 calls present in ROM
 * order. The verified source and the diff live in the lane evidence
 * (runs/linkw/out/w5a/evidence/hostcopy_verified/). When the decomp lands a
 * byte match for 0x0211b0a4, that TU replaces this file via slice_w5a.txt.
 *
 * Receiver note: both ClosestPlayer call sites already pass the actor (the
 * value the ROM leaves in r0) -- the Actor_ClosestPlayer_OverlayReaders.cpp
 * seam does not open here.
 *
 * The state dispatchers this calls for cases 5 and 0..2/6/9/10
 * (func_ov063_02116d38 / func_ov063_021192d4) are host copies too, in
 * Boo_CrossNamedDispatch.c -- their src TUs spell two callees by another
 * overlay's name (the shared-window naming race); see that file's banner.
 */

typedef int s32;
typedef short s16;
typedef unsigned int u32;
typedef unsigned short u16;
typedef signed char s8;
typedef unsigned char u8;
typedef struct 
{
  s32 x;
  s32 y;
  s32 z;
} Vector3;
typedef s32 Fix12;
typedef struct 
{
  char pad[0x50];
} RaycastGround;
extern void func_0200f760(void *a, void *b);
extern void *_ZN5Actor10FindWithIDEj(u32 id);
extern s32 _ZN8CapEnemy11GetCapStateEv(void *c);
extern s32 _ZN5Enemy26UpdateKillByInvincibleCharER12WithMeshClsnR9ModelAnimj(void *c, void *w, void *m, u32 j);
extern void _ZN5Actor8PoofDustEv(void *c);
extern void _ZN5Actor24KillAndTrackInDeathTableEv(void *c);
extern void func_0201267c(u32 a, void *b);
extern void *_ZN8CapEnemy15RespawnIfHasCapEv(void *c);
extern u8 IsAreaShowing(s8 idx);
extern s32 func_ov063_02116190(void *c);
extern void _ZN8CapEnemy12Unk_02005d94Ev(void *c);
extern s32 _ZN5Enemy14UpdateYoshiEatER12WithMeshClsn(void *c, void *w);
extern s32 _ZN8CapEnemy16GetCapEatenOffItERK7Vector3(void *c, const Vector3 *v);
extern void func_ov063_02119ab0(void *c);
extern s32 _ZN6Player11ShowMessageER9ActorBasejPK7Vector3jj(void *thiz, void *ab, u32 id, const void *pos, u32 e, u32 f);
extern s32 _ZN6Player12GetTalkStateEv(void *c);
extern void _ZN6Player9DropActorEv(void *c);
extern void func_ov063_021166ac(void *c);
extern void _ZN12CylinderClsn5ClearEv(void *c);
extern void *_ZN5Actor13ClosestPlayerEv(void *c);
extern s16 Vec3_HorzAngle(const Vector3 *a, const Vector3 *b);
extern Fix12 Vec3_HorzDist(const Vector3 *a, const Vector3 *b);
extern void func_ov063_021192d4(void *c);
extern void func_ov063_02116bf4(void *c);
extern void func_ov063_02116a1c(void *c);
extern void func_ov063_02116d38(void *c);
extern void func_ov063_0211ab68(void *c);
extern void func_ov063_021189f4(void *c);
extern void func_ov063_021172a8(void *c);
extern void func_ov063_02119274(void *c);
extern void func_ov063_02116fac(void *c);
extern u16 DecIfAbove0_Short(void *p);
extern void _ZN5Actor9UpdatePosEP12CylinderClsn(void *c, void *cyl);
extern void _ZN13RaycastGroundC1Ev(RaycastGround *rc);
extern void _ZN13RaycastGround12SetObjAndPosERK7Vector3P5Actor(RaycastGround *rc, const Vector3 *v, void *actor);
extern s32 _ZN13RaycastGround10DetectClsnEv(RaycastGround *rc);
extern void _ZN5Enemy12UpdateWMClsnER12WithMeshClsnj(void *c, void *w, u32 j);
extern void _ZN13RaycastGroundD1Ev(RaycastGround *rc);
extern void _ZN9Animation7AdvanceEv(void *c);
extern void _ZN25MovingCylinderClsnWithPos21SetPosRelativeToActorERK7Vector3(void *c, void *v);
extern void _ZN12CylinderClsn6UpdateEv(void *c);
s32 _ZN3Boo8BehaviorEv(void *arg0)
{
  char *c = (char *) arg0;
  Vector3 pv;
  Vector3 v1;
  Vector3 v2;
  Vector3 ve;
  RaycastGround rc1;
  RaycastGround rc2;
  s32 t;
  void *p;
  char *q;
  char *r1;
  char *r2;
  s32 *p19c;
  u16 *fp;
  u8 *bp;
  s32 y;
  s32 z;
  s32 x;
  int d1;
  int v;
  func_0200f760(c, c + 0x184);
  if ((*((u32 *) (c + 0x49c))) != 0)
  {
    *((void **) (c + 0x48c)) = _ZN5Actor10FindWithIDEj(*((u32 *) (c + 0x49c)));
    q = *((char **) (c + 0x48c));
    if (q != 0)
    {
      *((s32 *) (q + 0x5c)) = *((s32 *) (c + 0x5c));
      *((s32 *) (q + 0x60)) = *((s32 *) (c + 0x60));
      *((s32 *) (q + 0x64)) = *((s32 *) (c + 0x64));
    }
    *((void **) (c + 0x48c)) = 0;
  }
  if (_ZN8CapEnemy11GetCapStateEv(c) == 0)
  {
    return 1;
  }
  t = _ZN5Enemy26UpdateKillByInvincibleCharER12WithMeshClsnR9ModelAnimj(c, c + 0x1c4, c + 0x380, 0);
  if (t != 0)
  {
    if (t == 2)
    {
      _ZN5Actor8PoofDustEv(c);
      if ((*((s32 *) (c + 0x5a4))) != 0)
      {
        _ZN5Actor24KillAndTrackInDeathTableEv(c);
        func_0201267c(0xd5, c + 0x74);
        if (((*((u8 *) (c + 0x113))) & 0xf) < 6)
        {
          *((s32 *) (c + 0x5c)) = *((s32 *) (c + 0x51c));
          r1 = c + 0x500;
          *((s32 *) (c + 0x60)) = *((s32 *) (c + 0x520));
          r2 = (char *) (((long long) (c + 0x92)) & 0xFFFFFFFFFFFFFFFFLL);
          *((s32 *) (c + 0x64)) = *((s32 *) (c + 0x524));
          *((s8 *) (c + 0xcc)) = *((s8 *) (r1 + 0xd0));
          *((s16 *) (c + 0x92)) = *((s16 *) (r1 + 0x70));
          *((s16 *) (c + 0x94)) = *((s16 *) (r1 + 0x72));
          *((s16 *) (c + 0x96)) = *((s16 *) (r1 + 0x74));
          *((s16 *) (c + 0x8c)) = *((s16 *) r2);
          *((s16 *) (c + 0x8e)) = *((s16 *) (r2 + 2));
          *((s16 *) (c + 0x90)) = *((s16 *) (r2 + 4));
          p = _ZN8CapEnemy15RespawnIfHasCapEv(c);
          if (p != 0)
          {
            fp = (u16 *) (((long long) (((char *) p) + 0x5d4)) & 0xFFFFFFFFFFFFFFFFLL);
            *fp = (*fp) & (~2);
          }
        }
      }
      else
      {
        *((u8 *) (c + 0x5cc)) = 5;
        *((u8 *) (c + 0x5ce)) = 0;
        p19c = (s32 *) (((long long) (((char *) c) + 0x19c)) & 0xFFFFFFFFFFFFFFFFLL);
        *p19c = (*p19c) | 1;
        fp = (u16 *) (((long long) (((char *) c) + 0x5d4)) & 0xFFFFFFFFFFFFFFFFLL);
        *fp = (*fp) & (~8);
      }
    }
    return 1;
  }
  *((s8 *) (c + 0xcc)) = -1;
  if ((*((u16 *) (c + 0x5c6))) == 0)
  {
    if ((IsAreaShowing(*((s8 *) (c + 0x5d0))) == 0) || (func_ov063_02116190(c) != 0))
    {
      if ((((u32) ((*((u16 *) (c + 0x5d4))) << 0x18)) >> 0x1f) == 0)
      {
        *((s32 *) (c + 0x5c)) = *((s32 *) (c + 0x51c));
        fp = (u16 *) (((long long) (((char *) c) + 0x5d4)) & 0xFFFFFFFFFFFFFFFFLL);
        *((s32 *) (c + 0x60)) = *((s32 *) (c + 0x520));
        r2 = (char *) (((long long) (c + 0x92)) & 0xFFFFFFFFFFFFFFFFLL);
        *((s32 *) (c + 0x64)) = *((s32 *) (c + 0x524));
        r1 = c + 0x500;
        *((s16 *) (c + 0x92)) = *((s16 *) (r1 + 0x70));
        *((s16 *) (c + 0x94)) = *((s16 *) (r1 + 0x72));
        *((s16 *) (c + 0x96)) = *((s16 *) (r1 + 0x74));
        *((s16 *) (c + 0x8c)) = *((s16 *) r2);
        *((s16 *) (c + 0x8e)) = *((s16 *) (r2 + 2));
        *((s16 *) (c + 0x90)) = *((s16 *) (r2 + 4));
        *fp = (*fp) | 0x10;
        *((u8 *) (c + 0x5cc)) = 0;
        *((s32 *) (c + 0x98)) = 0;
        *((s32 *) (c + 0x9c)) = 0;
        *((s32 *) (c + 0xa8)) = 0;
        *((u8 *) (c + 0x5ce)) = 0;
        *((s8 *) (c + 0xcc)) = *((s8 *) (r1 + 0xd0));
      }
      return 1;
    }
  }
  _ZN8CapEnemy12Unk_02005d94Ev(c);
  t = _ZN5Enemy14UpdateYoshiEatER12WithMeshClsn(c, c + 0x1c4);
  if (t != 0)
  {
    if (t == 1)
    {
      ve.x = *((s32 *) (c + 0x564));
      ve.y = *((s32 *) (c + 0x568));
      ve.z = *((s32 *) (c + 0x56c));
      if (_ZN8CapEnemy16GetCapEatenOffItERK7Vector3(c, &ve) != 0)
      {
        return 1;
      }
    }
    *((s32 *) (c + 0x9c)) = -0x2000;
    func_ov063_02119ab0(c);
    if ((*((u8 *) (c + 0x107))) != 0)
    {
      u16 *p100 = (u16 *) (c + 0x100);
      if (p100[2] == 0)
      {
        *((u8 *) (c + 0x107)) = 0;
        ((u16 *) (c + 0x100))[2] = 0;
        *((s32 *) (c + 0x9c)) = 0;
        *((s32 *) (c + 0xa8)) = 0;
        goto block_39;
      }
    }
    v = ((*((s32 *) (c + 0xb0))) & 0x40000) ? (1) : (0);
    if (v != 0)
    {
      u8 st = *((u8 *) (c + 0x5d1));
      char *pl = *((char **) (c + 0xd0));
      switch (st)
      {
        case 0:
          if (_ZN6Player11ShowMessageER9ActorBasejPK7Vector3jj(pl, c, 0x15a, 0, 0, 2) != 0)
        {
          bp = (u8 *) ((((int) c) + 0x5d1) & 0xFFFFFFFFFFFFFFFF);
          *bp = (*bp) + 1;
          func_0201267c(0xf8, c + 0x74);
        }
          break;

        case 1:
          if (_ZN6Player12GetTalkStateEv(pl) == (-1))
        {
          _ZN6Player9DropActorEv(pl);
          bp = (u8 *) ((((int) c) + 0x5d1) & 0xFFFFFFFFFFFFFFFF);
          *bp = (*bp) + 1;
        }
          break;

      }

    }
    func_ov063_021166ac(c);
    _ZN12CylinderClsn5ClearEv(c + 0x184);
    return 1;
  }
  block_39:
  *((u8 *) (c + 0x5d1)) = 0;

  {
    void *plr = _ZN5Actor13ClosestPlayerEv(c);
    *((void **) (c + 0x484)) = plr;
    plr = *((void **) (c + 0x484));
    if (plr != 0)
    {
      s32 *r3 = (s32 *)(((char *) plr) + 0x5c);
      pv.x = r3[0];
      pv.y = r3[1];
      pv.z = r3[2];
      *((s16 *) (c + 0x5b0)) = Vec3_HorzAngle((Vector3 *) (c + 0x5c), &pv);
      *((s32 *) (c + 0x580)) = Vec3_HorzDist((Vector3 *) (c + 0x5c), &pv);
    }
    else
    {
      r1 = c + 0x500;
      *((s16 *) (r1 + 0xb0)) = *((s16 *) (c + 0x8e));
      *((s32 *) (c + 0x580)) = 0x2710000;
    }
  }
  *((u8 *) (c + 0x5cd)) = *((u8 *) (c + 0x5cc));
  switch (*((u8 *) (c + 0x5cf)))
  {
    case 0:

    case 1:

    case 2:
      func_ov063_021192d4(c);
      break;

    case 3:
      func_ov063_02116bf4(c);
      break;

    case 4:
      func_ov063_02116a1c(c);
      break;

    case 5:
      func_ov063_02116d38(c);
      break;

    case 6:

    case 10:
      func_ov063_021192d4(c);
      break;

    case 7:
      func_ov063_0211ab68(c);
      break;

    case 12:

    case 13:

    case 14:
      func_ov063_021189f4(c);
      break;

    case 15:
      func_ov063_021172a8(c);
      break;

    case 8:
      func_ov063_02119274(c);
      break;

    case 9:
      func_ov063_021192d4(c);
      break;

    case 11:
      func_ov063_02116fac(c);

  }

  if ((((u32) ((*((u16 *) (c + 0x5d4))) << 0x1d)) >> 0x1f) != 0)
  {
    *((s16 *) (c + 0x8e)) = *((s16 *) (c + 0x94));
  }
  *((u16 *) (c + 0x100)) = (*((u16 *) (c + 0x100))) + 1;
  DecIfAbove0_Short(c + 0x5c0);
  if ((*((u8 *) (c + 0x5cd))) != (*((u8 *) (c + 0x5cc))))
  {
    *((u16 *) (c + 0x100)) = 0;
  }
  if ((*((u8 *) (c + 0x5cf))) != 3)
  {
    _ZN5Actor9UpdatePosEP12CylinderClsn(c, c + 0x184);
    func_ov063_02119ab0(c);
    if (((((u32) ((*((u16 *) (c + 0x5d4))) << 0x1f)) >> 0x1f) != 0) && ((*((s32 *) (c + 0x64))) < (-0x12c000)))
    {
      *((s32 *) (c + 0x64)) = -0x12c000;
    }
    _ZN13RaycastGroundC1Ev(&rc1);
    y = *((s32 *) (c + 0x60));
    z = *((s32 *) (c + 0x64));
    x = *((s32 *) (c + 0x5c));
    v1.x = x;
    v1.y = y + 0x32000;
    v1.z = z;
    _ZN13RaycastGround12SetObjAndPosERK7Vector3P5Actor(&rc1, &v1, c);
    if (_ZN13RaycastGround10DetectClsnEv(&rc1) != 0)
    {
      s32 ground = (*((s32 *) (((char *) (&rc1)) + 0x44))) + 0x2000;
      if ((*((s32 *) (c + 0x60))) < ground)
      {
        *((s32 *) (c + 0x60)) = ground;
      }
    }
    if (((*((u8 *) (c + 0x5cf))) != 4) && ((*((u8 *) (c + 0x5cf))) != 0xb))
    {
      _ZN13RaycastGroundC1Ev(&rc2);
      y = *((s32 *) (c + 0x60));
      z = *((s32 *) (c + 0x64));
      x = *((s32 *) (c + 0x5c));
      v2.x = x;
      v2.y = y + 0x32000;
      v2.z = z;
      _ZN13RaycastGround12SetObjAndPosERK7Vector3P5Actor(&rc2, &v2, c);
      d1 = (int) ((*((u16 *) (c + 0xc))) == 0xd1);
      if (d1 != 0)
      {
        if ((*((u8 *) (c + 0x5cf))) < 8)
        {
          if ((((u32) ((*((u16 *) (c + 0x5d4))) << 0x1a)) >> 0x1f) != 0)
          {
            if ((_ZN13RaycastGround10DetectClsnEv(&rc2) == 0) || (((*((s32 *) (c + 0x60))) - (*((s32 *) (((char *) (&rc2)) + 0x44)))) > 0x12c000))
            {
              *((s32 *) (c + 0x5c)) = *((s32 *) (c + 0x528));
              *((s32 *) (c + 0x60)) = *((s32 *) (c + 0x52c));
              *((s32 *) (c + 0x64)) = *((s32 *) (c + 0x530));
            }
            else
            {
              *((s32 *) (c + 0x528)) = *((s32 *) (c + 0x5c));
              *((s32 *) (c + 0x52c)) = *((s32 *) (c + 0x60));
              *((s32 *) (c + 0x530)) = *((s32 *) (c + 0x64));
            }
          }
          else
          {
            goto ray_e;
          }
        }
        else
        {
          goto ray_e;
        }
      }
      else
      {
        ray_e:
        if ((_ZN13RaycastGround10DetectClsnEv(&rc2) != 0) && (((*((s32 *) (c + 0x60))) - (*((s32 *) (((char *) (&rc2)) + 0x44)))) < 0x12c000))
        {
          fp = (u16 *) (((long long) (((char *) c) + 0x5d4)) & 0xFFFFFFFFFFFFFFFFLL);
          *fp = (*fp) | 0x20;
          *((s32 *) (c + 0x528)) = *((s32 *) (c + 0x5c));
          *((s32 *) (c + 0x52c)) = *((s32 *) (c + 0x60));
          *((s32 *) (c + 0x530)) = *((s32 *) (c + 0x64));
        }

      }
      _ZN5Enemy12UpdateWMClsnER12WithMeshClsnj(c, c + 0x1c4, 0);
      _ZN13RaycastGroundD1Ev(&rc2);
    }
    _ZN13RaycastGroundD1Ev(&rc1);
  }
  if (((((*((u8 *) (c + 0x5cc))) != 3) && ((*((u8 *) (c + 0x5cc))) != 3)) && ((*((u8 *) (c + 0x5cc))) != 3)) && ((*((u8 *) (c + 0x5cc))) != 3))
  {
    _ZN9Animation7AdvanceEv(c + 0x3d0);
  }
  func_ov063_021166ac(c);
  _ZN12CylinderClsn5ClearEv(c + 0x184);
  if ((*((u8 *) (c + 0x5c8))) == 0xff)
  {
    _ZN25MovingCylinderClsnWithPos21SetPosRelativeToActorERK7Vector3(c + 0x184, c + 0x534);
    _ZN12CylinderClsn6UpdateEv(c + 0x184);
  }
  return 1;
}
