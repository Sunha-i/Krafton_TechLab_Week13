#pragma once
#include "SViewerWindow.h"

class SPhysicsAssetEditorWindow : public SViewerWindow
{
public:
    SPhysicsAssetEditorWindow();
    virtual ~SPhysicsAssetEditorWindow();

    virtual void OnRender() override;
    virtual void PreRenderViewportUpdate() override;

protected:
    virtual ViewerState* CreateViewerState(const char* Name, UEditorAssetPreviewContext* Context) override;
    virtual void DestroyViewerState(ViewerState*& State) override;
    virtual FString GetWindowTitle() const override { return "Physics Asset Editor"; }

private:
    // Load a skeletal mesh into the active tab
    void LoadSkeletalMesh(ViewerState* State, const FString& Path);

    // ImGui draw callback for viewport rendering
    static void ViewportRenderCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd);
};