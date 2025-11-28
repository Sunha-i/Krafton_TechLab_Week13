#pragma once
class ViewerState;
class UWorld;
struct ID3D11Device;

// Minimal bootstrap helpers to construct/destory per-tab viewer state.
class PhysicsAssetEditorBootstrap
{
public:
    static ViewerState* CreateViewerState(const char* Name, UWorld* InWorld, ID3D11Device* InDevice);
    static void DestroyViewerState(ViewerState*& State);
};