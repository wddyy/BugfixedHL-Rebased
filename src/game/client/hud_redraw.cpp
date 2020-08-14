/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// hud_redraw.cpp
//

extern "C"
{
#include <pm_shared.h>
}

#include <math.h>
#include "hud.h"
#include "cl_util.h"
#include "hud/spectator.h"
#include "vgui/client_viewport.h"

#define MAX_LOGO_FRAMES 56

int grgLogoFrame[MAX_LOGO_FRAMES] = {
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 13, 13, 13, 13, 13, 12, 11, 10, 9, 8, 14, 15,
	16, 17, 18, 19, 20, 20, 20, 20, 20, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
	29, 29, 29, 29, 29, 28, 27, 26, 25, 24, 30, 31
};

extern int g_iVisibleMouse;

float HUD_GetFOV(void);

extern cvar_t *sensitivity;

ConVar hud_colortext("hud_colortext", "1", FCVAR_BHL_ARCHIVE);

// Think
void CHud::Think(void)
{
	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo(&m_scrinfo);

	int newfov;

	for (CHudElem *i : m_HudList)
	{
		if (i->m_iFlags & HUD_ACTIVE)
			i->Think();
	}

	newfov = HUD_GetFOV();
	if (newfov == 0)
	{
		m_iFOV = default_fov->value;
	}
	else
	{
		m_iFOV = newfov;
	}

	// the clients fov is actually set in the client data update section of the hud

	// Set a new sensitivity
	if (m_iFOV == default_fov->value)
	{
		// reset to saved sensitivity
		m_flMouseSensitivity = 0;
	}
	else
	{
		// set a new sensitivity that is proportional to the change from the FOV default
		m_flMouseSensitivity = sensitivity->value * ((float)newfov / (float)default_fov->value) * CVAR_GET_FLOAT("zoom_sensitivity_ratio");
	}

	// think about default fov
	if (m_iFOV == 0)
	{ // only let players adjust up in fov,  and only if they are not overriden by something else
		m_iFOV = max(default_fov->value, 90.f);
	}

	if (gEngfuncs.IsSpectateOnly())
	{
		m_iFOV = CHudSpectator::Get()->GetFOV(); // default_fov->value;
	}

	// Update BHop state
	int bhopCapState = (int)GetBHopCapState();
	if (PM_GetBHopCapState() != bhopCapState)
		PM_SetBHopCapState(bhopCapState);

	// Update color code action
	int colorText = clamp(hud_colortext.GetInt(), 0, 2);
	m_ColorCodeAction = (ColorCodeAction)colorText;
	if (hud_colortext.GetInt() != colorText)
		hud_colortext.SetValue(colorText);
}

// Redraw
// step through the local data,  placing the appropriate graphics & text as appropriate
// returns 1 if they've changed, 0 otherwise
int CHud::Redraw(float flTime, int intermission)
{
	m_fOldTime = m_flTime; // save time of previous redraw
	m_flTime = flTime;
	m_flTimeDelta = (double)m_flTime - m_fOldTime;
	static float m_flShotTime = 0;

	// Clock was reset, reset delta
	if (m_flTimeDelta < 0)
		m_flTimeDelta = 0;

	UpdateHudColors();

	// Bring up the scoreboard during intermission
	if (g_pViewport)
	{
		if (m_iIntermission && !intermission)
		{
			// Have to do this here so the scoreboard goes away
			m_iIntermission = intermission;
			g_pViewport->HideCommandMenu();
			g_pViewport->HideScoreBoard();
			g_pViewport->UpdateSpectatorPanel();
		}
		else if (!m_iIntermission && intermission)
		{
			m_iIntermission = intermission;
			g_pViewport->HideCommandMenu();
			g_pViewport->HideAllVGUIMenu();
			g_pViewport->ShowScoreBoard();
			g_pViewport->UpdateSpectatorPanel();

			// Take a screenshot if the client's got the cvar set
			if (CVAR_GET_FLOAT("hud_takesshots") != 0)
				m_flShotTime = flTime + 1.0; // Take a screenshot in a second
		}
	}

	if (m_flShotTime && m_flShotTime < flTime)
	{
		gEngfuncs.pfnClientCmd("snapshot\n");
		m_flShotTime = 0;
	}

	m_iIntermission = intermission;

	// if no redrawing is necessary
	// return 0;

	// draw all registered HUD elements
	if (m_pCvarDraw->value)
	{
		for (CHudElem *i : m_HudList)
		{
			if (!intermission)
			{
				if ((i->m_iFlags & HUD_ACTIVE) && !(m_iHideHUDDisplay & HIDEHUD_ALL))
					i->Draw(flTime);
			}
			else
			{
				// it's an intermission, only draw hud elements that are set to draw during intermissions
				if (i->m_iFlags & HUD_INTERMISSION)
					i->Draw(flTime);
			}
		}
	}

	// are we in demo mode? do we need to draw the logo in the top corner?
	if (m_iLogo)
	{
		int x, y, i;

		if (m_hsprLogo == 0)
			m_hsprLogo = LoadSprite("sprites/%d_logo.spr");

		SPR_Set(m_hsprLogo, 250, 250, 250);

		x = SPR_Width(m_hsprLogo, 0);
		x = ScreenWidth - x;
		y = SPR_Height(m_hsprLogo, 0) / 2;

		// Draw the logo at 20 fps
		int iFrame = (int)(flTime * 20) % MAX_LOGO_FRAMES;
		i = grgLogoFrame[iFrame] - 1;

		SPR_DrawAdditive(i, x, y, NULL);
	}

	/*
	if ( g_iVisibleMouse )
	{
		void IN_GetMousePos( int *mx, int *my );
		int mx, my;

		IN_GetMousePos( &mx, &my );
		
		if (m_hsprCursor == 0)
		{
			char sz[256];
			sprintf( sz, "sprites/cursor.spr" );
			m_hsprCursor = SPR_Load( sz );
		}

		SPR_Set(m_hsprCursor, 250, 250, 250 );
		
		// Draw the logo at 20 fps
		SPR_DrawAdditive( 0, mx, my, NULL );
	}
	*/

	return 1;
}

void ScaleColors(int &r, int &g, int &b, int a)
{
	float x = (float)a / 255;
	r = (int)(r * x);
	g = (int)(g * x);
	b = (int)(b * x);
}

int CHud::DrawHudString(int xpos, int ypos, int iMaxX, char *szIt, int r, int g, int b)
{
	return xpos + gEngfuncs.pfnDrawString(xpos, ypos, szIt, r, g, b);
}

int CHud::DrawHudNumberString(int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b)
{
	char szString[32];
	sprintf(szString, "%d", iNumber);
	return DrawHudStringReverse(xpos, ypos, iMinX, szString, r, g, b);
}

// draws a string from right to left (right-aligned)
int CHud::DrawHudStringReverse(int xpos, int ypos, int iMinX, char *szString, int r, int g, int b)
{
	return xpos - gEngfuncs.pfnDrawStringReverse(xpos, ypos, szString, r, g, b);
}

int CHud::DrawHudStringColorCodes(int x, int y, int iMaxX, char *string, int _r, int _g, int _b)
{
	// How colorcodes work in DrawHudStringColorCodes
	// 1) Colorcodes are not ignored.
	// 2) Codes ^0 and ^9 reset color to _r, _g, _b.

	if (!string || !*string)
		return x;

	if (GetColorCodeAction() == ColorCodeAction::Ignore)
		return gHUD.DrawHudString(x, y, iMaxX, string, _r, _g, _b);

	char *c1 = string;
	char *c2 = string;
	int r = _r, g = _g, b = _b;
	int colorIndex;
	while (true)
	{
		// Search for next color code
		colorIndex = -1;
		while (*c2 && *(c2 + 1) && !IsColorCode(c2))
			c2++;
		if (IsColorCode(c2))
		{
			colorIndex = *(c2 + 1) - '0';
			*c2 = 0;
		}
		// Draw current string
		x = gHUD.DrawHudString(x, y, iMaxX, c1, r, g, b);

		if (colorIndex >= 0)
		{
			// Revert change and advance
			*c2 = '^';
			c2 += 2;
			c1 = c2;

			// Return if next string is empty
			if (!*c1)
				return x;

			// Setup color
			if (colorIndex == 0 || colorIndex == 9)
			{
				// Reset color
				r = _r;
				g = _g;
				b = _b;
			}
			else
			{
				r = gHUD.GetColorCodeColor(colorIndex)[0];
				g = gHUD.GetColorCodeColor(colorIndex)[1];
				b = gHUD.GetColorCodeColor(colorIndex)[2];
			}
			continue;
		}

		// Done
		break;
	}
	return x;
}

int CHud::DrawHudStringReverseColorCodes(int x, int y, int iMaxX, char *string, int _r, int _g, int _b)
{
	if (!string || !*string)
		return x;

	if (GetColorCodeAction() == ColorCodeAction::Ignore)
		return gHUD.DrawHudStringReverse(x, y, iMaxX, string, _r, _g, _b);

	// Move the string pos to the left to make it look like DrawHudStringReverse
	x -= TextMessageDrawString(ScreenWidth + 1, y, RemoveColorCodes(string), _r, _g, _b);

	return DrawHudStringColorCodes(x, y, iMaxX, string, _r, _g, _b);
}

int CHud::DrawHudNumber(int x, int y, int iFlags, int iNumber, int r, int g, int b)
{
	int iWidth = GetSpriteRect(m_HUD_number_0).right - GetSpriteRect(m_HUD_number_0).left;
	int k;

	if (iNumber > 0)
	{
		// SPR_Draw 100's
		if (iNumber >= 100)
		{
			k = iNumber / 100;
			SPR_Set(GetSprite(m_HUD_number_0 + k), r, g, b);
			SPR_DrawAdditive(0, x, y, &GetSpriteRect(m_HUD_number_0 + k));
			x += iWidth;
		}
		else if (iFlags & (DHN_3DIGITS))
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw 10's
		if (iNumber >= 10)
		{
			k = (iNumber % 100) / 10;
			SPR_Set(GetSprite(m_HUD_number_0 + k), r, g, b);
			SPR_DrawAdditive(0, x, y, &GetSpriteRect(m_HUD_number_0 + k));
			x += iWidth;
		}
		else if (iFlags & (DHN_3DIGITS | DHN_2DIGITS))
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw ones
		k = iNumber % 10;
		SPR_Set(GetSprite(m_HUD_number_0 + k), r, g, b);
		SPR_DrawAdditive(0, x, y, &GetSpriteRect(m_HUD_number_0 + k));
		x += iWidth;
	}
	else if (iFlags & DHN_DRAWZERO)
	{
		SPR_Set(GetSprite(m_HUD_number_0), r, g, b);

		// SPR_Draw 100's
		if (iFlags & (DHN_3DIGITS))
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		if (iFlags & (DHN_3DIGITS | DHN_2DIGITS))
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw ones

		SPR_DrawAdditive(0, x, y, &GetSpriteRect(m_HUD_number_0));
		x += iWidth;
	}

	return x;
}

int CHud::GetNumWidth(int iNumber, int iFlags)
{
	if (iFlags & (DHN_3DIGITS))
		return 3;

	if (iFlags & (DHN_2DIGITS))
		return 2;

	if (iNumber <= 0)
	{
		if (iFlags & (DHN_DRAWZERO))
			return 1;
		else
			return 0;
	}

	if (iNumber < 10)
		return 1;

	if (iNumber < 100)
		return 2;

	return 3;
}
