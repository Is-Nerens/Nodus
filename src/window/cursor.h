#pragma once

inline void NU_Internal_Set_Cursor_Default()
{
    SDL_SetCursor(GUI.cursorDefault);
}

inline void NU_Internal_Set_Cursor_Pointer()
{
    SDL_SetCursor(GUI.cursorPointer);
}

inline void NU_Internal_Set_Cursor_Text()
{
    SDL_SetCursor(GUI.cursorText);
}

inline void NU_Internal_Set_Cursor_Wait()
{
    SDL_SetCursor(GUI.cursorWait);
}

inline void NU_Internal_Set_Cursor_Crosshair()
{
    SDL_SetCursor(GUI.cursorCrosshair);
}

inline void NU_Internal_Set_Cursor_Move()
{
    SDL_SetCursor(GUI.cursorMove);
}

inline void NU_Internal_Set_Cursor_NsResize()
{
    SDL_SetCursor(GUI.cursorNsResize);
}

inline void NU_Internal_Set_Cursor_EwResize()
{
    SDL_SetCursor(GUI.cursorEwResize);
}

inline void NU_Internal_Set_Cursor_NwseResize()
{
    SDL_SetCursor(GUI.cursorNwseResize);
}

inline void NU_Internal_Set_Cursor_NeswResize()
{
    SDL_SetCursor(GUI.cursorNeswResize);
}