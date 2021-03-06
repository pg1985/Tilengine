/*
Tilengine - 2D Graphics library with raster effects
Copyright (c) 2015-2017 Marc Palacios Domenech (megamarc@hotmail.com)
All rights reserved.

Redistribution and use in source and binary forms, with or without modification 
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation 
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*!
 ******************************************************************************
 *
 * \file
 * \brief Tileset file loader (.tsx / .png) created with Tiled editor
 * http://www.mapeditor.org
 *
 * \author Megamarc
 * \date 20 sep 2015
 *
 * Public Tilengine source code
 * http://www.tilengine.org
 *
 ******************************************************************************
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "Tilengine.h"
#include "simplexml.h"
#include "LoadFile.h"

/* load manager */
struct
{
	char source[64];
	int tilewidth;
	int tileheight;
	int spacing;
	int margin;
}
static loader;

/* XML parser callback */
static void* handler (SimpleXmlParser parser, SimpleXmlEvent evt, 
	const char* szName, const char* szAttribute, const char* szValue)
{
	switch (evt)
	{
	case ADD_SUBTAG:
		break;

	case ADD_ATTRIBUTE:
		if (!strcasecmp(szAttribute, "source"))
			strcpy (loader.source, szValue);
		else if (!strcasecmp(szAttribute, "tilewidth"))
			loader.tilewidth = atoi(szValue);
		else if (!strcasecmp(szAttribute, "tileheight"))
			loader.tileheight = atoi(szValue);
		else if (!strcasecmp(szAttribute, "margin"))
			loader.margin = atoi(szValue);
		else if (!strcasecmp(szAttribute, "spacing"))
			loader.spacing = atoi(szValue);
		break;

	case FINISH_ATTRIBUTES:
		break;

	case ADD_CONTENT:
		break;

	case FINISH_TAG:
		break;
	}
	return handler;
}

/*!
 * \brief
 * Loads a tileset from a Tiled .tsx file
 * 
 * \param filename
 * TSX file to load
 * 
 * \returns
 * Reference to the newly loaded tileset or NULL if error
 *
 * \remarks
 * An associated palette is also created, it can be obtained calling TLN_GetTilesetPalette()
 */
TLN_Tileset TLN_LoadTileset (const char* filename)
{
	SimpleXmlParser parser;
	size_t size;
	uint8_t *data;
	TLN_Tileset tileset;
	TLN_Bitmap bitmap;
	int htiles, vtiles;
	int x,y,dx,dy;
	int id;
	int pitch;
	
	/* load file */
	data = LoadFile (filename, &size);
	if (!data)
	{
		if (size == 0)
			TLN_SetLastError (TLN_ERR_FILE_NOT_FOUND);
		else if (size == -1)
			TLN_SetLastError (TLN_ERR_OUT_OF_MEMORY);
		return NULL;
	}

	/* parse */
	memset (&loader, 0, sizeof(loader));
	parser = simpleXmlCreateParser (data, (long)size);
	if (parser != NULL)
	{
		if (simpleXmlParse(parser, handler) != 0)
		{
			printf("parse error on line %li:\n%s\n", 
				simpleXmlGetLineNumber(parser), simpleXmlGetErrorDescription(parser));
			free (data);
			TLN_SetLastError (TLN_ERR_WRONG_FORMAT);
			return NULL;
		}
	}
	else
		TLN_SetLastError (TLN_ERR_OUT_OF_MEMORY);

	free (data);

	/* check filename */
	if (!loader.source[0])
	{
		TLN_SetLastError (TLN_ERR_WRONG_FORMAT);
		return NULL;
	}
	
	/* load picture */
	bitmap = TLN_LoadBitmap (loader.source);
	if (!bitmap)
		return NULL;

	/* create tileset */
	dx = loader.tilewidth + loader.spacing;
	dy = loader.tileheight + loader.spacing;
	htiles = (TLN_GetBitmapWidth(bitmap) - loader.margin*2 + loader.spacing) / dx;
	vtiles = (TLN_GetBitmapHeight(bitmap) - loader.margin*2 + loader.spacing) / dy;
	tileset = TLN_CreateTileset (htiles*vtiles, loader.tilewidth, loader.tileheight, TLN_ClonePalette(TLN_GetBitmapPalette(bitmap)));
	pitch = TLN_GetBitmapPitch (bitmap);

	/* load tiles */
	for (id=1, y=0; y<vtiles; y++)
	{
		for (x=0; x<htiles; x++, id++)
		{
			uint8_t *srcptr = TLN_GetBitmapPtr (bitmap, loader.margin + x*dx, loader.margin + y*dy);
			TLN_SetTilesetPixels (tileset, id, srcptr, pitch);
		}
	}

	TLN_DeleteBitmap (bitmap);

	TLN_SetLastError (TLN_ERR_OK);
	return tileset;
}
