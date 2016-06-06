// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.

#include "client.h"

d_light_cache gDLightCache;
d_light_cache gDLightCacheDynamic;

extern d3d_render_pool_new gLMapPool;
extern d3d_render_pool_new gLMapPoolStatic;
extern player_info player;
extern d3d_driver_profile gD3DDriverProfile;
extern Bool gD3DRedrawAll;
extern particle_system gParticleSystemFireworks;
extern int gNumObjects;

void D3DLightingXYCalc(d_light *light, u_char intensity);
void D3DLightingColorCalc(d_light *light, u_short color);

/*
* D3DLightingRenderBegin: Called to set up lighting for rendering a frame.
*/
void D3DLightingRenderBegin(room_type *room)
{
   gDLightCache.numLights = 0;
   gDLightCacheDynamic.numLights = 0;
   D3DLMapsStaticGet(room);
}

/*
 * D3DLMapCheck: Returns true if light matches the given node.
 */
Bool D3DLMapCheck(d_light *dLight, room_contents_node *pRNode)
{
   if (dLight->objID != pRNode->obj.id)
      return FALSE;
   if (dLight->xyzScale.x != DLIGHT_SCALE(pRNode->obj.dLighting.intensity))
      return FALSE;
   if (dLight->color.b != (pRNode->obj.dLighting.color & 31) * 255 / 31)
      return FALSE;
   if (dLight->color.g != ((pRNode->obj.dLighting.color >> 5) & 31) * 255 / 31)
      return FALSE;
   if (dLight->color.r != ((pRNode->obj.dLighting.color >> 10) & 31) * 255 / 31)
      return FALSE;

   return TRUE;
}

/*
* D3DRenderLMapsPostDraw: For a given static light, traverse the BSP tree
*                          and light up any nodes it can illuminate.
*/
void D3DRenderLMapsPostDraw(BSPnode *tree, Draw3DParams *params, d_light *light)
{
   long side;

   if (!tree)
      return;

   // If the light isn't in this tree's bounding box, return.
   if (tree->bbox.x0 > light->maxX
      || tree->bbox.x1 < light->minX
      || tree->bbox.y0 > light->maxY
      || tree->bbox.y1 < light->minY)
      return;

   // If the player can't see the bounding box (behind camera view), return.
   if (IsHidden(params, (long)tree->bbox.x0, (long)tree->bbox.y0, (long)tree->bbox.x1, (long)tree->bbox.y1))
      return;

   switch (tree->type)
   {
   case BSPleaftype:
      if (tree->u.leaf.sector->flags & SF_HAS_ANIMATED)
      {
         //if (tree->drawfloor)
         D3DRenderLMapPostFloorAdd(tree, &gLMapPool, light, TRUE);
         //if (tree->drawceiling)
         D3DRenderLMapPostCeilingAdd(tree, &gLMapPool, light, TRUE);
      }
      return;

   case BSPinternaltype:
      side = tree->u.internal.separator.a * params->viewer_x +
         tree->u.internal.separator.b * params->viewer_y +
         tree->u.internal.separator.c;

      /* first, traverse closer side */
      if (side > 0)
         D3DRenderLMapsPostDraw(tree->u.internal.pos_side, params, light);
      else
         D3DRenderLMapsPostDraw(tree->u.internal.neg_side, params, light);

      /* then do walls on the separator */
      if (side != 0)
      {
         //WallList list;
         WallData *pWall;
         int flags;

         for (pWall = tree->u.internal.walls_in_plane; pWall != NULL; pWall = pWall->next)
         {
            if (!((pWall->pos_sidedef && pWall->pos_sidedef->flags & WF_HAS_ANIMATED)
               || (pWall->neg_sidedef && pWall->neg_sidedef->flags & WF_HAS_ANIMATED)
               || (pWall->pos_sector && pWall->pos_sector->flags & SF_HAS_ANIMATED)
               || (pWall->neg_sector && pWall->neg_sector->flags & SF_HAS_ANIMATED)))
               continue;

            flags = 0;
            if (pWall->pos_sidedef)
            {
               if (pWall->pos_sidedef->normal_bmap)
                  flags |= D3DRENDER_WALL_NORMAL;

               if (pWall->pos_sidedef->below_bmap)
                  flags |= D3DRENDER_WALL_BELOW;

               if (pWall->pos_sidedef->above_bmap)
                  flags |= D3DRENDER_WALL_ABOVE;
            }

            if (pWall->neg_sidedef)
            {
               if (pWall->neg_sidedef->normal_bmap)
                  flags |= D3DRENDER_WALL_NORMAL;

               if (pWall->neg_sidedef->below_bmap)
                  flags |= D3DRENDER_WALL_BELOW;

               if (pWall->neg_sidedef->above_bmap)
                  flags |= D3DRENDER_WALL_ABOVE;
            }

            pWall->separator.a = tree->u.internal.separator.a;
            pWall->separator.b = tree->u.internal.separator.b;
            pWall->separator.c = tree->u.internal.separator.c;

            // D3DRenderLMapsPostDraw now relies on checks done while constructing the list
            // of viewable objects, walls etc. in DrawBSP() in drawbsp.c. Walls that can be seen
            // have the respective boolean value set to TRUE, so all we need to do here is check
            // that value to decide whether to render light on the wall. Significantly speeds up
            // rendering large amounts of lights. Old code left commented out as opposed to deleting
            // in case any modification to drawbsp.c is performed which invalidates this method.
            //if (pWall->drawnormal) 
            if ((flags & D3DRENDER_WALL_NORMAL) && (((short)pWall->z2 != (short)pWall->z1)
               || (pWall->zz2 != pWall->zz1)))
            {
               D3DRenderLMapPostWallAdd(pWall, &gLMapPool, D3DRENDER_WALL_NORMAL, side, light, TRUE);
            }

            //if (pWall->drawbelow)
            if ((flags & D3DRENDER_WALL_BELOW) && (((short)pWall->z1 != (short)pWall->z0)
               || ((short)pWall->zz1 != (short)pWall->zz0)))
            {
               D3DRenderLMapPostWallAdd(pWall, &gLMapPool, D3DRENDER_WALL_BELOW, side, light, TRUE);
            }

            //if (pWall->drawabove)
            if ((flags & D3DRENDER_WALL_ABOVE) && (((short)pWall->z3 != (short)pWall->z2)
               || ((short)pWall->zz3 != (short)pWall->zz2)))
            {
               D3DRenderLMapPostWallAdd(pWall, &gLMapPool, D3DRENDER_WALL_ABOVE, side, light, TRUE);
            }
         }
      }

      /* lastly, traverse farther side */
      if (side > 0)
         D3DRenderLMapsPostDraw(tree->u.internal.neg_side, params, light);
      else
         D3DRenderLMapsPostDraw(tree->u.internal.pos_side, params, light);

      return;

   default:
      debug(("WalkBSPtree lightmaps error!\n"));
      return;
   }
}

/*
* D3DRenderLMapsDynamicPostDraw: For a given dynamic light, traverse the BSP tree
*                                and light up any nodes it can illuminate.
*/
void D3DRenderLMapsDynamicPostDraw(BSPnode *tree, Draw3DParams *params, d_light *light)
{
   long side;

   if (!tree)
      return;

   // If the light isn't in this tree's bounding box, return.
   if (tree->bbox.x0 > light->maxX
      || tree->bbox.x1 < light->minX
      || tree->bbox.y0 > light->maxY
      || tree->bbox.y1 < light->minY)
      return;

   // If the player can't see the bounding box (behind camera view), return.
   if (IsHidden(params, (long)tree->bbox.x0, (long)tree->bbox.y0,
      (long)tree->bbox.x1, (long)tree->bbox.y1))
      return;

   switch (tree->type)
   {
   case BSPleaftype:
      //if (tree->drawfloor)
      D3DRenderLMapPostFloorAdd(tree, &gLMapPool, light, TRUE);
      //if (tree->drawceiling)
      D3DRenderLMapPostCeilingAdd(tree, &gLMapPool, light, TRUE);

      return;

   case BSPinternaltype:
      side = tree->u.internal.separator.a * params->viewer_x +
         tree->u.internal.separator.b * params->viewer_y +
         tree->u.internal.separator.c;

      /* first, traverse closer side */
      if (side > 0)
         D3DRenderLMapsDynamicPostDraw(tree->u.internal.pos_side, params, light);
      else
         D3DRenderLMapsDynamicPostDraw(tree->u.internal.neg_side, params, light);

      /* then do walls on the separator */
      if (side != 0)
      {
         //WallList list;
         WallData *pWall;
         int flags;

         for (pWall = tree->u.internal.walls_in_plane; pWall != NULL; pWall = pWall->next)
         {
            flags = 0;

            if (pWall->pos_sidedef)
            {

               if (pWall->pos_sidedef->normal_bmap)
                  flags |= D3DRENDER_WALL_NORMAL;

               if (pWall->pos_sidedef->below_bmap)
                  flags |= D3DRENDER_WALL_BELOW;

               if (pWall->pos_sidedef->above_bmap)
                  flags |= D3DRENDER_WALL_ABOVE;
            }

            if (pWall->neg_sidedef)
            {
               if (pWall->neg_sidedef->normal_bmap)
                  flags |= D3DRENDER_WALL_NORMAL;

               if (pWall->neg_sidedef->below_bmap)
                  flags |= D3DRENDER_WALL_BELOW;

               if (pWall->neg_sidedef->above_bmap)
                  flags |= D3DRENDER_WALL_ABOVE;
            }

            pWall->separator.a = tree->u.internal.separator.a;
            pWall->separator.b = tree->u.internal.separator.b;
            pWall->separator.c = tree->u.internal.separator.c;

            // D3DRenderLMapsDynamicPostDraw now relies on checks done while constructing the list
            // of viewable objects, walls etc. in DrawBSP() in drawbsp.c. Walls that can be seen
            // have the respective boolean value set to TRUE, so all we need to do here is check
            // that value to decide whether to render light on the wall. Significantly speeds up
            // rendering large amounts of lights. Old code left commented out as opposed to deleting
            // in case any modification to drawbsp.c is performed which invalidates this method.
            //if (pWall->drawnormal)
            if ((flags & D3DRENDER_WALL_NORMAL) && (((short)pWall->z2 != (short)pWall->z1)
               || ((short)pWall->zz2 != (short)pWall->zz1)))
            {
               D3DRenderLMapPostWallAdd(pWall, &gLMapPool, D3DRENDER_WALL_NORMAL, side, light, TRUE);
            }

            //if (pWall->drawbelow) 
            if ((flags & D3DRENDER_WALL_BELOW) && (((short)pWall->z1 != (short)pWall->z0)
               || ((short)pWall->zz1 != (short)pWall->zz0)))
            {
               D3DRenderLMapPostWallAdd(pWall, &gLMapPool, D3DRENDER_WALL_BELOW, side, light, TRUE);
            }

            //if (pWall->drawabove)
            if ((flags & D3DRENDER_WALL_ABOVE) && (((short)pWall->z3 != (short)pWall->z2)
               || ((short)pWall->zz3 != (short)pWall->zz2)))
            {
               D3DRenderLMapPostWallAdd(pWall, &gLMapPool, D3DRENDER_WALL_ABOVE, side, light, TRUE);
            }
         }
      }

      /* lastly, traverse farther side */
      if (side > 0)
         D3DRenderLMapsDynamicPostDraw(tree->u.internal.neg_side, params, light);
      else
         D3DRenderLMapsDynamicPostDraw(tree->u.internal.pos_side, params, light);

      return;

   default:
      debug(("WalkBSPtree lightmaps error!\n"));
      return;
   }
}

/*
* D3DLMapsStaticGet: Iterate through possible lighting sources
*                    and add them to the light caches.
*/
void D3DLMapsStaticGet(room_type *room)
{
   room_contents_node *pRNode;
   list_type list;
   long top, bottom;
   int sector_flags;
   PDIB pDib;

   for (list = room->projectiles; list != NULL; list = list->next)
   {
      Projectile *pProjectile = (Projectile *)list->data;

      if (gDLightCacheDynamic.numLights >= MAX_NUM_DLIGHTS
         || pProjectile->dLighting.color == 0
         || pProjectile->dLighting.intensity == 0)
         continue;

      gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].flags = (int)pProjectile->dLighting.flags;
      gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].xyz.x = pProjectile->motion.x;
      gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].xyz.y = pProjectile->motion.y;
      gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].xyz.z = pProjectile->motion.z;

      pDib = GetObjectPdib(pProjectile->icon_res, 0, 0);

      GetRoomHeight(room->tree, &top, &bottom, &sector_flags, pProjectile->motion.x, pProjectile->motion.y);

      gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].xyz.z =
         max(bottom, pProjectile->motion.z);

      if (pDib)
         gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].xyz.z +=
         ((float)pDib->height / (float)pDib->shrink * 16.0f) - (float)pDib->yoffset * 4.0f;

      D3DLightingXYCalc(&gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights],
         pProjectile->dLighting.intensity);
      D3DLightingColorCalc(&gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights],
         pProjectile->dLighting.color);

      gDLightCacheDynamic.numLights++;
   }

   for (list = room->contents; list != NULL; list = list->next)
   {
      pRNode = (room_contents_node *)list->data;

      // If user has an object targeted, add a highlight under the object
      // using the same color as the user's selected target halo.
      if (pRNode->obj.id != INVALID_ID
         && pRNode->obj.id != player.id
         && config.target_highlight
         && pRNode->obj.id == GetUserTargetID()
         && pRNode->obj.drawingtype != DRAWFX_INVISIBLE
         && gDLightCacheDynamic.numLights < MAX_NUM_DLIGHTS)
      {
         gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].flags =
            LIGHT_FLAG_DYNAMIC | LIGHT_FLAG_ON | LIGHT_FLAG_HIGHLIGHT;
         gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].xyz.x = pRNode->motion.x;
         gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].xyz.y = pRNode->motion.y;

         GetRoomHeight(room->tree, &top, &bottom, &sector_flags, pRNode->motion.x, pRNode->motion.y);
         gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].xyz.z = bottom;

         D3DLightingXYCalc(&gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights], COLOR_MAX);

         u_short light_color;
         switch (config.halocolor)
         {
         case TARGET_COLOR_RED:
            light_color = LIGHT_BRED; // Red
            break;
         case TARGET_COLOR_BLUE:
            light_color = LIGHT_BBLUE; // Blue
            break;
         default: // TARGET_COLOR_GREEN
            light_color = LIGHT_BGREEN; // Green
         }

         D3DLightingColorCalc(&gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights], light_color);

         gDLightCacheDynamic.numLights++;
      }

      // dynamic lights
      if (pRNode->obj.dLighting.flags & LIGHT_FLAG_DYNAMIC)
      {
         if (gDLightCacheDynamic.numLights >= MAX_NUM_DLIGHTS
            || pRNode->obj.dLighting.color == 0
            || pRNode->obj.dLighting.intensity == 0)
            continue;

         gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].flags = (int)pRNode->obj.dLighting.flags;
         gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].xyz.x = pRNode->motion.x;
         gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].xyz.y = pRNode->motion.y;
         gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].xyz.z = pRNode->motion.z;

         pDib = GetObjectPdib(pRNode->obj.icon_res, 0, 0);

         GetRoomHeight(room->tree, &top, &bottom, &sector_flags, pRNode->motion.x, pRNode->motion.y);

         gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].xyz.z =
            max(bottom, pRNode->motion.z);

         if (pDib)
            gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].xyz.z +=
            ((float)pDib->height / (float)pDib->shrink * 16.0f) - (float)pDib->yoffset * 4.0f;

         D3DLightingXYCalc(&gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights],
            pRNode->obj.dLighting.intensity);
         D3DLightingColorCalc(&gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights],
            pRNode->obj.dLighting.color);

         gDLightCacheDynamic.numLights++;
      }
      else
      {
         // static lights
         if (gDLightCache.numLights >= MAX_NUM_DLIGHTS
            || pRNode->obj.dLighting.color == 0
            || pRNode->obj.dLighting.intensity == 0)
            continue;

         if (!D3DLMapCheck(&gDLightCache.dLights[gDLightCache.numLights], pRNode))
            gD3DRedrawAll |= D3DRENDER_REDRAW_ALL;

         pDib = GetObjectPdib(pRNode->obj.icon_res, 0, 0);

         gDLightCache.dLights[gDLightCache.numLights].objID = pRNode->obj.id;
         gDLightCacheDynamic.dLights[gDLightCache.numLights].flags = (int)pRNode->obj.dLighting.flags;
         gDLightCache.dLights[gDLightCache.numLights].xyz.x = pRNode->motion.x;
         gDLightCache.dLights[gDLightCache.numLights].xyz.y = pRNode->motion.y;
         gDLightCache.dLights[gDLightCache.numLights].xyz.z = pRNode->motion.z;

         GetRoomHeight(room->tree, &top, &bottom, &sector_flags, pRNode->motion.x, pRNode->motion.y);

         gDLightCache.dLights[gDLightCache.numLights].xyz.z =
            max(bottom, pRNode->motion.z);

         if (pDib)
            gDLightCache.dLights[gDLightCache.numLights].xyz.z +=
            ((float)pDib->height / (float)pDib->shrink * 16.0f) - (float)pDib->yoffset * 4.0f;

         D3DLightingXYCalc(&gDLightCache.dLights[gDLightCache.numLights],
            pRNode->obj.dLighting.intensity);
         D3DLightingColorCalc(&gDLightCache.dLights[gDLightCache.numLights],
            pRNode->obj.dLighting.color);

         if (pRNode->obj.dLighting.intensity == 0)
            pRNode->obj.dLighting.intensity = 1;

         gDLightCache.numLights++;
      }
   }

   // fireworks
   if (effects.fireworks)
   {
      emitter *pEmitter;
      particle *pParticle;
      for (list = gParticleSystemFireworks.emitterList; list != NULL; list = list->next)
      {
         pEmitter = (emitter *)list->data;
         pParticle = &pEmitter->particles[0];
         if (pParticle && pParticle->energy > 0)
         {
            if (gDLightCacheDynamic.numLights >= MAX_NUM_DLIGHTS)
               continue;
            gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].flags = 0;
            gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].xyz.x = pParticle->pos.x;
            gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].xyz.y = pParticle->pos.y;
            gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].xyz.z = pParticle->pos.z;

            D3DLightingXYCalc(&gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights],
               pParticle->energy);

            // Particles have their own rgb color already, use this.
            gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].color.a = COLOR_MAX;
            gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].color.r = pParticle->bgra.r;
            gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].color.g = pParticle->bgra.g;
            gDLightCacheDynamic.dLights[gDLightCacheDynamic.numLights].color.b = pParticle->bgra.b;

            gDLightCacheDynamic.numLights++;
         }
      }
   }
}

/*
* D3DLightingXYCalc: Perform lighting xyz calculations based on intensity.
*/
void D3DLightingXYCalc(d_light *light, u_char intensity)
{
   if (light->flags & LIGHT_FLAG_HIGHLIGHT)
      light->xyzScale.x = light->xyzScale.y = light->xyzScale.z = DLIGHT_SCALE(intensity) * 0.1f;
   else
      light->xyzScale.x = light->xyzScale.y = light->xyzScale.z = DLIGHT_SCALE(intensity);

   light->maxX = light->xyz.x + light->xyzScale.x / 2.0f;
   light->minX = light->xyz.x - light->xyzScale.x / 2.0f;
   light->maxY = light->xyz.y + light->xyzScale.y / 2.0f;
   light->minY = light->xyz.y - light->xyzScale.y / 2.0f;

   light->invXYZScale.x = 1.0f / light->xyzScale.x;
   light->invXYZScale.y = 1.0f / light->xyzScale.y;
   light->invXYZScale.z = 1.0f / light->xyzScale.z;

   light->invXYZScaleHalf.x = 1.0f / (light->xyzScale.x / 2.0f);
   light->invXYZScaleHalf.y = 1.0f / (light->xyzScale.y / 2.0f);
   light->invXYZScaleHalf.z = 1.0f / (light->xyzScale.z / 2.0f);
}

/*
* D3DLightingColorCalc: Convert the blakserv 2-byte lighting color
*                       to rgb color values.
*/
void D3DLightingColorCalc(d_light *light, u_short color)
{
   light->color.a = COLOR_MAX;
   light->color.r = ((color >> 10) & 31) * COLOR_MAX / 31;
   light->color.g = ((color >> 5) & 31) * COLOR_MAX / 31;
   light->color.b = (color & 31) * COLOR_MAX / 31;
}

/*
* D3DRenderLMapPostFloorAdd: Adds render data for a floor with lighting.
*/
void D3DRenderLMapPostFloorAdd(BSPnode *pNode, d3d_render_pool_new *pPool,
                               d_light *light, Bool bDynamic)
{
   Sector *pSector = pNode->u.leaf.sector;

   if (!pSector || !pSector->floor)
      return;

   PDIB pDib = pSector->floor;

   int count;
   float falloff;

   custom_st st[MAX_NPTS];
   custom_bgra bgra[MAX_NPTS];

   d3d_render_packet_new *pPacket;
   d3d_render_chunk_new *pChunk;

   pPacket = D3DRenderPacketFindMatch(pPool, NULL, pDib, 0, 0, 0);
   if (NULL == pPacket)
      return;
   pChunk = D3DRenderChunkNew(pPacket);
   assert(pChunk);

   pChunk->numVertices = pNode->u.leaf.poly.npts;
   pChunk->numIndices = pChunk->numVertices;
   pChunk->numPrimitives = pChunk->numVertices - 2;
   pChunk->pSector = pSector;
   pPacket->pMaterialFctn = &D3DMaterialLMapDynamicPacket;
   pChunk->side = 0;

   if (pSector->light <= 127)
      pChunk->flags |= D3DRENDER_NOAMBIENT;

   if (bDynamic)
      pChunk->pMaterialFctn = &D3DMaterialLMapDynamicChunk;
   else
      pChunk->pMaterialFctn = &D3DMaterialLMapStaticChunk;

   for (count = 0; count < pNode->u.leaf.poly.npts; count++)
   {
      bgra[count] = pNode->floor_bgra[count];

      falloff = (pNode->floor_xyz[count].z - light->xyz.z) * light->invXYZScaleHalf.z;

      if (falloff < 0)
         falloff = -falloff;

      falloff = min(1.0f, falloff);
      falloff = 1.0f - falloff;
      // falloff = 255.0f * falloff;

      if (light->flags & LIGHT_FLAG_HIGHLIGHT)
         falloff = 1.0f;

      bgra[count].b = falloff * light->color.b;
      bgra[count].g = falloff * light->color.g;
      bgra[count].r = falloff * light->color.r;
      bgra[count].a = falloff * light->color.a;

      st[count].s = pNode->floor_xyz[count].x - light->xyz.x;
      st[count].t = pNode->floor_xyz[count].y - light->xyz.y;

      st[count].s *= light->invXYZScale.x;
      st[count].t *= light->invXYZScale.y;

      st[count].s += 0.5f;
      st[count].t += 0.5f;

      pChunk->xyz[count].x = pNode->floor_xyz[count].x;
      pChunk->xyz[count].y = pNode->floor_xyz[count].y;
      pChunk->xyz[count].z = pNode->floor_xyz[count].z;

      pChunk->bgra[count].b = bgra[count].b;
      pChunk->bgra[count].g = bgra[count].g;
      pChunk->bgra[count].r = bgra[count].r;
      pChunk->bgra[count].a = bgra[count].a;

      pChunk->st0[count].s = st[count].s;
      pChunk->st0[count].t = st[count].t;

      pChunk->st1[count].s = pNode->floor_stBase[count].s;
      pChunk->st1[count].t = pNode->floor_stBase[count].t;
   }

   unsigned int index;
   int first, last;

   first = 1;
   last = pChunk->numVertices - 1;

   pChunk->indices[0] = 0;
   pChunk->indices[1] = last--;
   pChunk->indices[2] = first++;

   for (index = 3; index < pChunk->numIndices; first++, last--, index += 2)
   {
      pChunk->indices[index] = last;
      pChunk->indices[index + 1] = first;
   }
   gNumObjects++;
}

/*
* D3DRenderLMapPostCeilingAdd: Adds render data for a ceiling with lighting.
*/
void D3DRenderLMapPostCeilingAdd(BSPnode *pNode, d3d_render_pool_new *pPool,
                                 d_light *light, Bool bDynamic)
{
   Sector *pSector = pNode->u.leaf.sector;

   if (!pSector || !pSector->ceiling)
      return;

   PDIB pDib = pSector->ceiling;

   custom_bgra bgra[MAX_NPTS];
   custom_st st[MAX_NPTS];
   float falloff;
   int count;

   d3d_render_packet_new *pPacket;
   d3d_render_chunk_new *pChunk;

   pPacket = D3DRenderPacketFindMatch(pPool, NULL, pDib, 0, 0, 0);
   if (NULL == pPacket)
      return;

   pChunk = D3DRenderChunkNew(pPacket);
   assert(pChunk);

   pChunk->numVertices = pNode->u.leaf.poly.npts;
   pChunk->numIndices = pChunk->numVertices;
   pChunk->numPrimitives = pChunk->numVertices - 2;
   pChunk->pSector = pSector;
   pPacket->pMaterialFctn = &D3DMaterialLMapDynamicPacket;
   pChunk->side = 0;

   if (pSector->light <= 127)
      pChunk->flags |= D3DRENDER_NOAMBIENT;

   if (bDynamic)
      pChunk->pMaterialFctn = &D3DMaterialLMapDynamicChunk;
   else
      pChunk->pMaterialFctn = &D3DMaterialLMapStaticChunk;

   for (count = 0; count < pNode->u.leaf.poly.npts; count++)
   {
      bgra[count] = pNode->ceiling_bgra[count];

      falloff = (pNode->ceiling_xyz[count].z - light->xyz.z) * light->invXYZScaleHalf.z;

      if (falloff < 0)
         falloff = -falloff;

      falloff = min(1.0f, falloff);
      falloff = 1.0f - falloff;
      // falloff = 255.0f * falloff;

      bgra[count].b = falloff * light->color.b;
      bgra[count].g = falloff * light->color.g;
      bgra[count].r = falloff * light->color.r;
      bgra[count].a = falloff * light->color.a;

      st[count].s = pNode->ceiling_xyz[count].x - light->xyz.x;
      st[count].t = pNode->ceiling_xyz[count].y - light->xyz.y;

      st[count].s *= light->invXYZScale.x;
      st[count].t *= light->invXYZScale.y;

      st[count].s += 0.5f;
      st[count].t += 0.5f;

      pChunk->xyz[count].x = pNode->ceiling_xyz[count].x;
      pChunk->xyz[count].y = pNode->ceiling_xyz[count].y;
      pChunk->xyz[count].z = pNode->ceiling_xyz[count].z;

      pChunk->bgra[count].b = bgra[count].b;
      pChunk->bgra[count].g = bgra[count].g;
      pChunk->bgra[count].r = bgra[count].r;
      pChunk->bgra[count].a = bgra[count].a;

      pChunk->st0[count].s = st[count].s;
      pChunk->st0[count].t = st[count].t;

      pChunk->st1[count].s = pNode->ceiling_stBase[count].s;
      pChunk->st1[count].t = pNode->ceiling_stBase[count].t;

      unsigned int index;
      int first, last;

      first = 1;
      last = pChunk->numVertices - 1;

      pChunk->indices[0] = 0;
      pChunk->indices[1] = first++;
      pChunk->indices[2] = last--;

      for (index = 3; index < pChunk->numIndices; first++, last--, index += 2)
      {
         pChunk->indices[index] = first;
         pChunk->indices[index + 1] = last;
      }
   }
   gNumObjects++;
}

/*
* D3DRenderLMapPostWallAdd: Adds render data for a wall with lighting.
*/
void D3DRenderLMapPostWallAdd(WallData *pWall, d3d_render_pool_new *pPool,
                              unsigned int type, int side, d_light *light,
                              Bool bDynamic)
{
   custom_xyz xyz[4];
   custom_st st[4];
   custom_st stBase[4];
   custom_bgra bgra[4];
   unsigned int flags;
   PDIB pDib = NULL;
   Sidedef *pSideDef = NULL;

   d3d_render_packet_new *pPacket;
   d3d_render_chunk_new *pChunk;

   // pos and neg sidedefs have their x and y coords reversed
   if (side > 0)
   {
      pSideDef = pWall->pos_sidedef;

      if (NULL == pSideDef)
         return;

      switch (type)
      {
      case D3DRENDER_WALL_NORMAL:
         if (pWall->pos_sidedef->normal_bmap)
         {
            pDib = pWall->pos_sidedef->normal_bmap;
            for (int i = 0; i < 4; i++)
            {
               xyz[i] = pWall->pos_normal_xyz[i];
               bgra[i] = pWall->pos_normal_bgra[i];
               stBase[i] = pWall->pos_normal_stBase[i];
               flags = pWall->pos_normal_d3dFlags;
            }
         }
         else
            pDib = NULL;
         break;

      case D3DRENDER_WALL_BELOW:
         if (pWall->pos_sidedef->below_bmap)
         {
            pDib = pWall->pos_sidedef->below_bmap;
            for (int i = 0; i < 4; i++)
            {
               xyz[i] = pWall->pos_below_xyz[i];
               bgra[i] = pWall->pos_below_bgra[i];
               stBase[i] = pWall->pos_below_stBase[i];
               flags = pWall->pos_below_d3dFlags;
            }
         }
         else
            pDib = NULL;
         break;

      case D3DRENDER_WALL_ABOVE:
         if (pWall->pos_sidedef->above_bmap)
         {
            pDib = pWall->pos_sidedef->above_bmap;
            for (int i = 0; i < 4; i++)
            {
               xyz[i] = pWall->pos_above_xyz[i];
               bgra[i] = pWall->pos_above_bgra[i];
               stBase[i] = pWall->pos_above_stBase[i];
               flags = pWall->pos_above_d3dFlags;
            }
         }
         else
            pDib = NULL;
         break;

      default:
         break;
      }
   }
   else if (side < 0)
   {
      pSideDef = pWall->neg_sidedef;

      if (NULL == pSideDef)
         return;

      switch (type)
      {
      case D3DRENDER_WALL_NORMAL:
         if (pWall->neg_sidedef->normal_bmap)
         {
            pDib = pWall->neg_sidedef->normal_bmap;
            for (int i = 0; i < 4; i++)
            {
               xyz[i] = pWall->neg_normal_xyz[i];
               bgra[i] = pWall->neg_normal_bgra[i];
               stBase[i] = pWall->neg_normal_stBase[i];
               flags = pWall->neg_normal_d3dFlags;
            }
         }
         else
            pDib = NULL;
         break;

      case D3DRENDER_WALL_BELOW:
         if (pWall->neg_sidedef->below_bmap)
         {
            pDib = pWall->neg_sidedef->below_bmap;
            for (int i = 0; i < 4; i++)
            {
               xyz[i] = pWall->neg_below_xyz[i];
               bgra[i] = pWall->neg_below_bgra[i];
               stBase[i] = pWall->neg_below_stBase[i];
               flags = pWall->neg_below_d3dFlags;
            }
         }
         else
            pDib = NULL;
         break;

      case D3DRENDER_WALL_ABOVE:
         if (pWall->neg_sidedef->above_bmap)
         {
            pDib = pWall->neg_sidedef->above_bmap;
            for (int i = 0; i < 4; i++)
            {
               xyz[i] = pWall->neg_above_xyz[i];
               bgra[i] = pWall->neg_above_bgra[i];
               stBase[i] = pWall->neg_above_stBase[i];
               flags = pWall->neg_above_d3dFlags;
            }
         }
         else
            pDib = NULL;
         break;

      default:
         break;
      }
   }

   if (NULL == pDib)
      return;

   unsigned int i;
   float falloff;
   custom_xyz normal, vec0, vec1;

   // calc cross product, get normal and determine major axis
   vec0.x = xyz[1].x - xyz[0].x;
   vec0.y = xyz[1].y - xyz[0].y;
   vec0.z = xyz[1].z - xyz[0].z;

   vec1.x = xyz[3].x - xyz[0].x;
   vec1.y = xyz[3].y - xyz[0].y;
   vec1.z = xyz[3].z - xyz[0].z;

   normal.x = vec0.z * vec1.y - vec0.y * vec1.z;
   normal.z = vec0.y * vec1.x - vec0.x * vec1.y;
   normal.y = vec0.x * vec1.z - vec0.z * vec1.x;

   // check to see if dot product is necessary
   //			if ((light->xyz.z < xyz[0].z) &&
   //				(light->xyz.z > xyz[1].z))

   // Disabled, causes some polys not to be lit when they should be.
   /*if (0)
   {
   custom_xyz	lightVec;
   float		cosAngle;

   lightVec.x = light->xyz.x - xyz[0].x;
   lightVec.y = light->xyz.y - xyz[0].y;
   lightVec.z = light->xyz.z - xyz[0].z;

   cosAngle = lightVec.x * normal.x +
   lightVec.y * normal.y +
   lightVec.z * normal.z;

   if (cosAngle <= 0)
   continue;
   }*/

   pPacket = D3DRenderPacketFindMatch(pPool, NULL, pDib, 0, 0, 0);
   if (NULL == pPacket)
      return;
   pChunk = D3DRenderChunkNew(pPacket);
   assert(pChunk);

   pChunk->flags = flags;
   pChunk->numIndices = 4;
   pChunk->numVertices = 4;
   pChunk->numPrimitives = pChunk->numVertices - 2;
   pChunk->pSideDef = pSideDef;
   pChunk->pSectorPos = pWall->pos_sector;
   pChunk->pSectorNeg = pWall->neg_sector;
   pChunk->side = side;
   pPacket->pMaterialFctn = &D3DMaterialLMapDynamicPacket;

   if (bDynamic)
      pChunk->pMaterialFctn = &D3DMaterialLMapDynamicChunk;
   else
      pChunk->pMaterialFctn = &D3DMaterialLMapStaticChunk;

   if (normal.x < 0)
      normal.x = -normal.x;

   if (normal.y < 0)
      normal.y = -normal.y;

   if (normal.x > normal.y)
   {
      for (i = 0; i < 4; i++)
      {
         falloff = (xyz[i].x - light->xyz.x) * light->invXYZScaleHalf.x;

         if (falloff < 0)
            falloff = -falloff;

         falloff = min(1.0f, falloff);
         falloff = 1.0f - falloff;

         st[i].s = (xyz[i].y - light->xyz.y) * light->invXYZScale.y + 0.5f;
         st[i].t = (xyz[i].z - light->xyz.z) * light->invXYZScale.z + 0.5f;
         bgra[i].b = falloff * light->color.b;
         bgra[i].g = falloff * light->color.g;
         bgra[i].r = falloff * light->color.r;
         bgra[i].a = falloff * light->color.a;
      }
   }
   else
   {
      for (i = 0; i < 4; i++)
      {
         falloff = (xyz[i].y - light->xyz.y) * light->invXYZScaleHalf.y;

         if (falloff < 0)
            falloff = -falloff;

         falloff = min(1.0f, falloff);
         falloff = 1.0f - falloff;

         st[i].s = (xyz[i].x - light->xyz.x) * light->invXYZScale.x + 0.5f;
         st[i].t = (xyz[i].z - light->xyz.z) * light->invXYZScale.z + 0.5f;
         bgra[i].b = falloff * light->color.b;
         bgra[i].g = falloff * light->color.g;
         bgra[i].r = falloff * light->color.r;
         bgra[i].a = falloff * light->color.a;
      }
   }

   for (i = 0; i < pChunk->numVertices; i++)
   {
      pChunk->xyz[i].x = xyz[i].x;
      pChunk->xyz[i].y = xyz[i].y;
      pChunk->xyz[i].z = xyz[i].z;

      pChunk->bgra[i].b = bgra[i].b;
      pChunk->bgra[i].g = bgra[i].g;
      pChunk->bgra[i].r = bgra[i].r;
      pChunk->bgra[i].a = bgra[i].a;

      pChunk->st0[i].s = st[i].s;
      pChunk->st0[i].t = st[i].t;

      pChunk->st1[i].s = stBase[i].s;
      pChunk->st1[i].t = stBase[i].t;
   }

   pChunk->indices[0] = 1;
   pChunk->indices[1] = 2;
   pChunk->indices[2] = 0;
   pChunk->indices[3] = 3;

   //			for (i = 0; i < 4; i++)
   //				CACHE_ST_ADD(pRenderCache, 1, stBase[i].s, stBase[i].t);
}

/*
* D3DRenderObjectGetLight: Returns the light level for the sector occupied
*                          by the node.
*/
int D3DRenderObjectGetLight(BSPnode *tree, room_contents_node *pRNode)
{
   long side0;
   BSPnode *pos, *neg;

   while (1)
   {
      if (tree == NULL)
      {
         return False;
      }

      switch (tree->type)
      {
      case BSPleaftype:
         return tree->u.leaf.sector->light;

      case BSPinternaltype:
         side0 = tree->u.internal.separator.a * pRNode->motion.x +
            tree->u.internal.separator.b * pRNode->motion.y +
            tree->u.internal.separator.c;

         pos = tree->u.internal.pos_side;
         neg = tree->u.internal.neg_side;

         if (side0 == 0)
            tree = (pos != NULL) ? pos : neg;
         else if (side0 > 0)
            tree = pos;
         else if (side0 < 0)
            tree = neg;
         break;

      default:
         debug(("add_object error!\n"));
         return False;
      }
   }
}

/*
* D3DRenderObjectLightGetNearest: Get the distance to the nearest light from the
*                                 given node. Not currently called by anything.
*/
float D3DRenderObjectLightGetNearest(room_contents_node *pRNode)
{
   int		numLights;
   float	lastDistance, distance;

   lastDistance = 0;

   for (numLights = 0; numLights < gDLightCache.numLights; numLights++)
   {
      custom_xyz	vector;

      vector.x = pRNode->motion.x - gDLightCache.dLights[numLights].xyz.x;
      vector.y = pRNode->motion.y - gDLightCache.dLights[numLights].xyz.y;
      vector.z = (pRNode->motion.z - gDLightCache.dLights[numLights].xyz.z);// * 4.0f;

      distance = (vector.x * vector.x) + (vector.y * vector.y) +
         (vector.z * vector.z);
      distance = (float)sqrt((double)distance);

      distance /= (gDLightCache.dLights[numLights].xyzScale.x / 2.0f);

      if (0 == numLights)
         lastDistance = distance;
      else if (distance < lastDistance)
         lastDistance = distance;
   }

   for (numLights = 0; numLights < gDLightCacheDynamic.numLights; numLights++)
   {
      custom_xyz	vector;

      vector.x = pRNode->motion.x - gDLightCacheDynamic.dLights[numLights].xyz.x;
      vector.y = pRNode->motion.y - gDLightCacheDynamic.dLights[numLights].xyz.y;
      vector.z = (pRNode->motion.z - gDLightCacheDynamic.dLights[numLights].xyz.z);// * 4.0f;

      distance = (vector.x * vector.x) + (vector.y * vector.y) +
         (vector.z * vector.z);
      distance = (float)sqrt((double)distance);

      distance /= (gDLightCacheDynamic.dLights[numLights].xyzScale.x / 2.0f);

      if (distance < lastDistance)
         lastDistance = distance;
   }

   if (gDLightCache.numLights || gDLightCacheDynamic.numLights)
   {
      lastDistance = 1.0f - lastDistance;
      lastDistance = max(0, lastDistance);
      //		lastDistance = 255 * lastDistance;
   }

   return lastDistance;
}

/*
* D3DObjectLightingCalc: Light calcs. Gets the nearest light to an object and
                         puts the color data into bgra. Returns bFogDisable.
*/
Bool D3DObjectLightingCalc(room_type *room, room_contents_node *pRNode, custom_bgra *bgra, DWORD flags)
{
   int light, intDistance, numLights;
   d_light *pDLight = NULL;
   float distX, distY;
   float lastDistance, distance;
   Bool bFogDisable = FALSE;

   lastDistance = DLIGHT_SCALE(255);

   for (numLights = 0; numLights < gDLightCache.numLights; numLights++)
   {
      custom_xyz vector;

      vector.x = pRNode->motion.x - gDLightCache.dLights[numLights].xyz.x;
      vector.y = pRNode->motion.y - gDLightCache.dLights[numLights].xyz.y;
      vector.z = (pRNode->motion.z - gDLightCache.dLights[numLights].xyz.z);// * 4.0f;

      distance = sqrt((vector.x * vector.x) + (vector.y * vector.y)
                      + (vector.z * vector.z));
      distance /= (gDLightCache.dLights[numLights].xyzScale.x / 2.0f);

      if (distance < lastDistance)
      {
         lastDistance = distance;
         pDLight = &gDLightCache.dLights[numLights];
      }
   }

   for (numLights = 0; numLights < gDLightCacheDynamic.numLights; numLights++)
   {
      custom_xyz vector;

      vector.x = pRNode->motion.x - gDLightCacheDynamic.dLights[numLights].xyz.x;
      vector.y = pRNode->motion.y - gDLightCacheDynamic.dLights[numLights].xyz.y;
      vector.z = (pRNode->motion.z - gDLightCacheDynamic.dLights[numLights].xyz.z);// * 4.0f;

      distance = sqrt((vector.x * vector.x) + (vector.y * vector.y)
                      + (vector.z * vector.z));
      distance /= (gDLightCacheDynamic.dLights[numLights].xyzScale.x / 2.0f);

      if (distance < lastDistance)
      {
         lastDistance = distance;
         pDLight = &gDLightCacheDynamic.dLights[numLights];
      }
   }

   lastDistance = COLOR_AMBIENT * max(0, 1.0f - lastDistance);

   light = D3DRenderObjectGetLight(room->tree, pRNode);

   if (light <= 127)
      bFogDisable = TRUE;

   distX = pRNode->motion.x - player.x;
   distY = pRNode->motion.y - player.y;

   intDistance = DistanceGet(distX, distY);

   if (gD3DDriverProfile.bFogEnable && ((flags & D3DRENDER_NOAMBIENT) == 0))
      intDistance = FINENESS;

   if (pRNode->obj.flags & OF_FLASHING)
      light = GetLightPaletteIndex(intDistance, light, FINENESS, pRNode->obj.lightAdjust);
   else
      light = GetLightPaletteIndex(intDistance, light, FINENESS, 0);

   light = light * COLOR_AMBIENT / 64;

   if (pDLight)
   {
      bgra->b = bgra->g = bgra->r = min(COLOR_AMBIENT, light);
      bgra->a = 255;

      bgra->b = min(COLOR_AMBIENT, bgra->b + (lastDistance * pDLight->color.b / COLOR_AMBIENT));
      bgra->g = min(COLOR_AMBIENT, bgra->g + (lastDistance * pDLight->color.g / COLOR_AMBIENT));
      bgra->r = min(COLOR_AMBIENT, bgra->r + (lastDistance * pDLight->color.r / COLOR_AMBIENT));
   }
   else
   {
      bgra->b = bgra->g = bgra->r = min(COLOR_AMBIENT, light + lastDistance);
      bgra->a = 255;
   }

   return bFogDisable;
}
