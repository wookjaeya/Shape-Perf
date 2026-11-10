/* imgbudget.c -- the budget of the static image of one object (kz-static extract --image-budget N): where a part of
 * the initializer that has a leaf is not read because the budget is used up, the image ends that part in one marker
 * leaf "<image truncated>"; an exact fit has none.  Read in the profile-free mode with --image-budget 4, one case at a
 * time (-DIMG_CASE=1 .. 5) and all five together (IMG_CASE not defined).  Each object reaches GLOBAL_IMAGES as the
 * argument of a call. */
int img_use(const void *p); /* no body */

#if !defined(IMG_CASE) || IMG_CASE == 1
/* 1: a record with more members than the budget: a .. d read, e and f not (marker .e...) */
struct ImgWide {
  int a;
  int b;
  int c;
  int d;
  int e;
  int f;
};
struct ImgWide img_wide = {1, 2, 3, 4, 5, 6};
#endif

#if !defined(IMG_CASE) || IMG_CASE == 2
/* 2: a union whose active member is a record with more members than the budget (marker .s.e...) */
union ImgUnion {
  struct {
    int a;
    int b;
    int c;
    int d;
    int e;
  } s;
  long x;
};
union ImgUnion img_union = {{1, 2, 3, 4, 5}};
#endif

#if !defined(IMG_CASE) || IMG_CASE == 3
/* 3: records inside an array: [0] read, [1].x read, [1].y on not (marker [1].y...), [2] not (marker [...]) */
struct ImgPoint {
  int x;
  int y;
  int z;
};
struct ImgPoint img_points[3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
#endif

#if !defined(IMG_CASE) || IMG_CASE == 4
/* 4: an exact fit: a .. d take the whole budget; the array member after them has no initializer, so it has no leaf
 * and nothing is left out (no marker, the budget used up) */
struct ImgExact {
  int a;
  int b;
  int c;
  int d;
  int rest[2];
};
struct ImgExact img_exact = {.a = 1, .b = 2, .c = 3, .d = 4};
#endif

#if !defined(IMG_CASE) || IMG_CASE == 5
/* 5: under the budget: three leaves */
struct ImgSmall {
  int a;
  int b;
  int c;
};
struct ImgSmall img_small = {1, 2, 3};
#endif

int img_main(void) {
  int n = 0;
#if !defined(IMG_CASE) || IMG_CASE == 1
  n += img_use(&img_wide);
#endif
#if !defined(IMG_CASE) || IMG_CASE == 2
  n += img_use(&img_union);
#endif
#if !defined(IMG_CASE) || IMG_CASE == 3
  n += img_use(&img_points[0]);
#endif
#if !defined(IMG_CASE) || IMG_CASE == 4
  n += img_use(&img_exact);
#endif
#if !defined(IMG_CASE) || IMG_CASE == 5
  n += img_use(&img_small);
#endif
  return n;
}
