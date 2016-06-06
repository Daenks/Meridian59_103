// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
#ifndef __D3DLIGHTING_H__
#define __D3DLIGHTING_H__

#define DLIGHT_SCALE(_x)		((_x * 14000 / 255) + 4000)

#define MAX_NUM_DLIGHTS 150

typedef struct d_light
{
   custom_xyz xyz;
   // Max and min x,y values for how far
   // the light reaches.
   float maxX, maxY, minX, minY;
   custom_xyz xyzScale;
   custom_xyz invXYZScale;
   custom_xyz invXYZScaleHalf;
   custom_bgra color;
   ID objID;
   int flags;
} d_light;

typedef struct d_light_cache
{
   int numLights;
   d_light dLights[MAX_DLIGHTS];
} d_light_cache;

void D3DLightingRenderBegin(room_type *room);

// Light map functions.
Bool D3DLMapCheck(d_light *dLight, room_contents_node *pRNode);
void D3DRenderLMapPostFloorAdd(BSPnode *pNode, d3d_render_pool_new *pPool, d_light *light, Bool bDynamic);
void D3DRenderLMapPostCeilingAdd(BSPnode *pNode, d3d_render_pool_new *pPool, d_light *light, Bool bDynamic);
void D3DRenderLMapPostWallAdd(WallData *pWall, d3d_render_pool_new *pPool, unsigned int type, int side, d_light *light, Bool bDynamic);
void D3DRenderLMapsPostDraw(BSPnode *tree, Draw3DParams *params, d_light *light);
void D3DRenderLMapsDynamicPostDraw(BSPnode *tree, Draw3DParams *params, d_light *light);
void D3DLMapsStaticGet(room_type *room);

int D3DRenderObjectGetLight(BSPnode *tree, room_contents_node *pRNode);
float D3DRenderObjectLightGetNearest(room_contents_node *pRNode);
Bool D3DObjectLightingCalc(room_type *room, room_contents_node *pRNode, custom_bgra *bgra, DWORD flags);


#endif
