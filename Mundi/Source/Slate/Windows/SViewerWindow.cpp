#include "pch.h"
#include "SViewerWindow.h"
#include "FViewport.h"
#include "FViewportClient.h"
#include "SelectionManager.h"
#include "SlateManager.h"
#include "Source/Editor/FBXLoader.h"
#include "Source/Editor/PlatformProcess.h"
#include "Source/Runtime/Engine/GameFramework/SkeletalMeshActor.h"
#include "Source/Runtime/Engine/GameFramework/CameraActor.h"
#include "Source/Runtime/Engine/Viewer/EditorAssetPreviewContext.h"
#include "Texture.h"
#include "Gizmo/GizmoActor.h"
#include "ResourceManager.h"

SViewerWindow::SViewerWindow()
{
}

SViewerWindow::~SViewerWindow()
{
}

bool SViewerWindow::Initialize(float StartX, float StartY, float Width, float Height, UWorld* InWorld, ID3D11Device* InDevice, UEditorAssetPreviewContext* InContext)
{
    World = InWorld;
    Device = InDevice;
    Context = InContext;

    // Create window title
    FString BaseTitle = GetWindowTitle();
    if (Context && !Context->AssetPath.empty())
    {
        // ex) "Data/Mesh/MyAsset.fbx" -> "MyAsset.fbx"
        std::filesystem::path fsPath(Context->AssetPath);
        FString AssetName = fsPath.filename().string();
        WindowTitle = BaseTitle + " - " + AssetName;
    }
    else
    {
        WindowTitle = BaseTitle;
    }

    SetRect(StartX, StartY, StartX + Width, StartY + Height);

    // 뷰어 툴바 아이콘 로드
    LoadViewerToolbarIcons(Device);

    // Create first tab/state
    OpenNewTab("Viewer 1");
    if (ActiveState && ActiveState->Viewport)
    {
        ActiveState->Viewport->Resize((uint32)StartX, (uint32)StartY, (uint32)Width, (uint32)Height);
    }

    bRequestFocus = true;
    return true;
}

void SViewerWindow::OnUpdate(float DeltaSeconds)
{
    if (!ActiveState || !ActiveState->Viewport)
        return;

    // Tick the preview world so editor actors (e.g., gizmo) update visibility/state
    if (ActiveState->World)
    {
        ActiveState->World->Tick(DeltaSeconds);

        // 뷰어 윈도우가 포커스되었을 때만 키보드 입력으로 기즈모 모드 전환
        if (ActiveState->World->GetGizmoActor() && bIsWindowFocused)
            ActiveState->World->GetGizmoActor()->ProcessGizmoModeSwitch();
    }

    if (ActiveState && ActiveState->Client)
    {
        ActiveState->Client->Tick(DeltaSeconds);
    }
}

void SViewerWindow::OnMouseMove(FVector2D MousePos)
{
    if (!ActiveState || !ActiveState->Viewport) return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);
        ActiveState->Viewport->ProcessMouseMove((int32)LocalPos.X, (int32)LocalPos.Y);
    }
}

void SViewerWindow::OnMouseDown(FVector2D MousePos, uint32 Button)
{
    if (!ActiveState || !ActiveState->Viewport) return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);

        // First, always try gizmo picking (pass to viewport)
        ActiveState->Viewport->ProcessMouseButtonDown((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);

        // Left click: if no gizmo was picked, try bone picking
        if (Button == 0 && ActiveState->PreviewActor && ActiveState->CurrentMesh && ActiveState->Client && ActiveState->World)
        {
            // Check if gizmo was picked by checking selection
            UActorComponent* SelectedComp = ActiveState->World->GetSelectionManager()->GetSelectedComponent();

            // Only do bone picking if gizmo wasn't selected
            if (!SelectedComp || !Cast<UBoneAnchorComponent>(SelectedComp))
            {
                // Get camera from viewport client
                ACameraActor* Camera = ActiveState->Client->GetCamera();
                if (Camera)
                {
                    // Get camera vectors
                    FVector CameraPos = Camera->GetActorLocation();
                    FVector CameraRight = Camera->GetRight();
                    FVector CameraUp = Camera->GetUp();
                    FVector CameraForward = Camera->GetForward();

                    // Calculate viewport-relative mouse position
                    FVector2D ViewportMousePos(MousePos.X - CenterRect.Left, MousePos.Y - CenterRect.Top);
                    FVector2D ViewportSize(CenterRect.GetWidth(), CenterRect.GetHeight());

                    // Generate ray from mouse position
                    FRay Ray = MakeRayFromViewport(
                        Camera->GetViewMatrix(),
                        Camera->GetProjectionMatrix(CenterRect.GetWidth() / CenterRect.GetHeight(), ActiveState->Viewport),
                        CameraPos,
                        CameraRight,
                        CameraUp,
                        CameraForward,
                        ViewportMousePos,
                        ViewportSize
                    );

                    // Try to pick a bone
                    float HitDistance;
                    int32 PickedBoneIndex = ActiveState->PreviewActor->PickBone(Ray, HitDistance);

                    if (PickedBoneIndex >= 0)
                    {
                        // Bone was picked
                        ActiveState->SelectedBoneIndex = PickedBoneIndex;
                        ActiveState->bBoneLinesDirty = true;
                        ActiveState->bRequestScrollToBone = true;

                        ExpandToSelectedBone(ActiveState, PickedBoneIndex);

                        // Move gizmo to the selected bone
                        ActiveState->PreviewActor->RepositionAnchorToBone(PickedBoneIndex);
                        if (USceneComponent* Anchor = ActiveState->PreviewActor->GetBoneGizmoAnchor())
                        {
                            ActiveState->World->GetSelectionManager()->SelectActor(ActiveState->PreviewActor);
                            ActiveState->World->GetSelectionManager()->SelectComponent(Anchor);
                        }
                    }
                    else
                    {
                        // No bone was picked - clear selection
                        ActiveState->SelectedBoneIndex = -1;
                        ActiveState->SelectedNotify.Invalidate();
                        ActiveState->bBoneLinesDirty = true;

                        // Hide gizmo and clear selection
                        if (UBoneAnchorComponent* Anchor = ActiveState->PreviewActor->GetBoneGizmoAnchor())
                        {
                            Anchor->SetVisibility(false);
                            Anchor->SetEditability(false);
                        }
                        ActiveState->World->GetSelectionManager()->ClearSelection();
                    }
                }
            }
        }

        // 우클릭: 카메라 조작 시작 (커서 숨김 및 잠금)
        if (Button == 1)
        {
            INPUT.SetCursorVisible(false);
            INPUT.LockCursor();
            bRightMousePressed = true;
        }
    }
}

void SViewerWindow::OnMouseUp(FVector2D MousePos, uint32 Button)
{
    if (!ActiveState || !ActiveState->Viewport) return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);
        ActiveState->Viewport->ProcessMouseButtonUp((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);

        // 우클릭 해제: 커서 복원 및 잠금 해제
        if (Button == 1 && bRightMousePressed)
        {
            INPUT.SetCursorVisible(true);
            INPUT.ReleaseCursor();
            bRightMousePressed = false;
        }
    }
}

void SViewerWindow::OnRenderViewport()
{
    if (ActiveState && ActiveState->Viewport && CenterRect.GetWidth() > 0 && CenterRect.GetHeight() > 0)
    {
        // Clamp viewport to window bounds to prevent integer wraparound and corruption
        extern float CLIENTWIDTH;
        extern float CLIENTHEIGHT;

        float ClampedLeft = std::max(0.0f, CenterRect.Left);
        float ClampedTop = std::max(0.0f, CenterRect.Top);
        float ClampedRight = std::min(CLIENTWIDTH, CenterRect.Right);
        float ClampedBottom = std::min(CLIENTHEIGHT, CenterRect.Bottom);

        // Skip rendering if viewport is completely outside window bounds
        if (ClampedLeft >= ClampedRight || ClampedTop >= ClampedBottom)
            return;

        const uint32 NewStartX = static_cast<uint32>(ClampedLeft);
        const uint32 NewStartY = static_cast<uint32>(ClampedTop);
        const uint32 NewWidth = static_cast<uint32>(ClampedRight - ClampedLeft);
        const uint32 NewHeight = static_cast<uint32>(ClampedBottom - ClampedTop);
        ActiveState->Viewport->Resize(NewStartX, NewStartY, NewWidth, NewHeight);

        // Child class specific update logic
        PreRenderViewportUpdate();

        // Viewport rendering (before ImGui)
        ActiveState->Viewport->Render();
    }
}

void SViewerWindow::OpenNewTab(const char* Name)
{
    ViewerState* State = CreateViewerState(Name, Context);
    if (!State) return;

    Tabs.Add(State);
    ActiveTabIndex = Tabs.Num() - 1;
    ActiveState = State;
}

void SViewerWindow::CloseTab(int Index)
{
    if (Index < 0 || Index >= Tabs.Num()) return;
    ViewerState* State = Tabs[Index];
    DestroyViewerState(State);
    Tabs.RemoveAt(Index);
    if (Tabs.Num() == 0) { ActiveTabIndex = -1; ActiveState = nullptr; }
    else { ActiveTabIndex = std::min(Index, Tabs.Num() - 1); ActiveState = Tabs[ActiveTabIndex]; }
}

void SViewerWindow::RenderLeftPanel(float PanelWidth)
{
    if (!ActiveState)   return;

    ImGuiStyle& style = ImGui::GetStyle();
    float spacing = style.ItemSpacing.y;

    // Asset Browser Section
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.30f, 0.30f, 0.30f, 0.8f));
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::Text("ASSET BROWSER");
    ImGui::PopFont();
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 6));
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.35f, 0.35f, 0.35f, 0.6f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 6));
    ImGui::Spacing();

    // Mesh path section
    ImGui::BeginGroup();
    ImGui::PushItemWidth(-1.0f);
    ImGui::InputTextWithHint("##MeshPath", "Browse for FBX file...", ActiveState->MeshPathBuffer, sizeof(ActiveState->MeshPathBuffer));
    ImGui::PopItemWidth();
    ImGui::Dummy(ImVec2(0, 4));
    ImGui::Spacing();

    // Buttons
    float innerPadding = 8.0f;  // 좌우 여백
    float availWidth = PanelWidth - innerPadding * 2.0f;
    float buttonHeight = 30.0f;
    float buttonWidth = (availWidth - 6.0f) * 0.5f;

    // Browse...
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.23f, 0.25f, 0.27f, 1.00f)); // #3A3F45
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.32f, 0.34f, 1.00f)); // #485057
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.20f, 0.21f, 1.00f)); // #2E3337
    if (ImGui::Button("Browse...", ImVec2(buttonWidth, buttonHeight)))
    {
        auto widePath = FPlatformProcess::OpenLoadFileDialog(
            UTF8ToWide(GDataDir), L"fbx", L"FBX Files");

        if (!widePath.empty())
        {
            std::string s = widePath.string();
            strncpy_s(ActiveState->MeshPathBuffer, s.c_str(),
                sizeof(ActiveState->MeshPathBuffer) - 1);
        }
    }

    ImGui::PopStyleColor(3);
    ImGui::SameLine(0.0f, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.29f, 0.44f, 0.63f, 1.00f)); // #4A6FA0
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.36f, 0.51f, 0.72f, 1.00f)); // #5B82B8
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.22f, 0.36f, 0.51f, 1.00f)); // #385B82

    if (ImGui::Button("Load FBX", ImVec2(buttonWidth, buttonHeight)))
    {
        FString Path = ActiveState->MeshPathBuffer;
        if (!Path.empty())
        {
            USkeletalMesh* Mesh = UResourceManager::GetInstance().Load<USkeletalMesh>(Path);
            if (Mesh && ActiveState->PreviewActor)
            {
                ActiveState->PreviewActor->SetSkeletalMesh(Path);
                ActiveState->CurrentMesh = Mesh;

                // Expand all bone nodes by default on mesh load
                ActiveState->ExpandedBoneIndices.clear();
                if (const FSkeleton* Skeleton = ActiveState->CurrentMesh->GetSkeleton())
                {
                    // Auto-load an animation from the same FBX (if available) for convenience
                    if (UAnimSequence* Anim = UFbxLoader::GetInstance().LoadFbxAnimation(Path, Skeleton))
                    {
                        ActiveState->CurrentAnimation = Anim;
                        ActiveState->TotalTime = Anim->GetSequenceLength();
                        ActiveState->CurrentTime = 0.0f;
                        ActiveState->bIsPlaying = true;

                        // Use the settings of ActiveState
                        ActiveState->PreviewActor
                            ->GetSkeletalMeshComponent()
                            ->PlayAnimation(Anim, ActiveState->bIsLooping, ActiveState->PlaybackSpeed);
                    }
                    for (int32 i = 0; i < Skeleton->Bones.size(); ++i)
                    {
                        ActiveState->ExpandedBoneIndices.insert(i);
                    }
                }

                ActiveState->LoadedMeshPath = Path;  // Track for resource unloading
                if (auto* Skeletal = ActiveState->PreviewActor->GetSkeletalMeshComponent())
                {
                    Skeletal->SetVisibility(ActiveState->bShowMesh);
                }
                ActiveState->bBoneLinesDirty = true;
                if (auto* LineComp = ActiveState->PreviewActor->GetBoneLineComponent())
                {
                    LineComp->ClearLines();
                    LineComp->SetLineVisible(ActiveState->bShowBones);
                }
            }
        }
    }
    ImGui::PopStyleColor(3);
    ImGui::EndGroup();

    // Section divider
    ImGui::Dummy(ImVec2(0, 4));
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.40f, 0.40f, 0.40f, 0.80f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 8));
    ImGui::Spacing();

    // Display Options
    ImGui::BeginGroup();

    // Title
    ImGui::Text("DISPLAY OPTIONS");
    ImGui::Dummy(ImVec2(0, 4));
    // Checkbox Style
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.5, 1.5));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.23f, 0.25f, 0.27f, 0.80f)); // #3A3F45 계열
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.28f, 0.30f, 0.33f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.20f, 0.22f, 0.25f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.75f, 0.80f, 0.90f, 1.00f));

    // Checkboxes
    if (ImGui::Checkbox("Show Mesh", &ActiveState->bShowMesh))
    {
        if (auto* comp = ActiveState->PreviewActor->GetSkeletalMeshComponent())
            comp->SetVisibility(ActiveState->bShowMesh);
    }

    ImGui::SameLine(0.0f, 12.0f);

    if (ImGui::Checkbox("Show Bones", &ActiveState->bShowBones))
    {
        if (auto* lineComp = ActiveState->PreviewActor->GetBoneLineComponent())
            lineComp->SetLineVisible(ActiveState->bShowBones);

        if (ActiveState->bShowBones)
            ActiveState->bBoneLinesDirty = true;
    }

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();
    ImGui::EndGroup();

    // Section divider
    ImGui::Dummy(ImVec2(0, 8));
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.40f, 0.40f, 0.40f, 0.75f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 8));

    // Bone Hierarchy Section
    ImGui::Text("BONE HIERARCHY");
    ImGui::Dummy(ImVec2(0, 2));
    ImGui::Spacing();

    if (!ActiveState->CurrentMesh)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("No skeletal mesh loaded.");
        ImGui::PopStyleColor();
    }
    else
    {
        const FSkeleton* Skeleton = ActiveState->CurrentMesh->GetSkeleton();
        if (!Skeleton || Skeleton->Bones.IsEmpty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::TextWrapped("This mesh has no skeleton data.");
            ImGui::PopStyleColor();
        }
        else
        {
            // Scrollable tree view
            ImGui::BeginChild("BoneTreeView", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 12.0f);
            const TArray<FBone>& Bones = Skeleton->Bones;
            TArray<TArray<int32>> Children;
            Children.resize(Bones.size());
            for (int32 i = 0; i < Bones.size(); ++i)
            {
                int32 Parent = Bones[i].ParentIndex;
                if (Parent >= 0 && Parent < Bones.size())
                {
                    Children[Parent].Add(i);
                }
            }

            std::function<void(int32)> DrawNode = [&](int32 Index)
                {
                    const bool bLeaf = Children[Index].IsEmpty();
                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;

                    if (bLeaf)
                    {
                        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                    }

                    // 펼쳐진 노드는 명시적으로 열린 상태로 설정
                    if (ActiveState->ExpandedBoneIndices.count(Index) > 0)
                    {
                        ImGui::SetNextItemOpen(true);
                    }

                    if (ActiveState->SelectedBoneIndex == Index)
                    {
                        flags |= ImGuiTreeNodeFlags_Selected;
                    }

                    ImGui::PushID(Index);
                    const char* Label = Bones[Index].Name.c_str();

                    if (ActiveState->SelectedBoneIndex == Index)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.35f, 0.55f, 0.85f, 0.8f));
                        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.40f, 0.60f, 0.90f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.30f, 0.50f, 0.80f, 1.0f));
                    }

                    bool open = ImGui::TreeNodeEx((void*)(intptr_t)Index, flags, "%s", Label ? Label : "<noname>");

                    if (ActiveState->SelectedBoneIndex == Index)
                    {
                        ImGui::PopStyleColor(3);

                        if (ActiveState->bRequestScrollToBone)
                        {
                            ImGui::SetScrollHereY(0.5f);    // 선택된 본까지 스크롤
                            ActiveState->bRequestScrollToBone = false; // 요청 처리 완료
                        }
                    }

                    // 사용자가 수동으로 노드를 접거나 펼쳤을 때 상태 업데이트
                    if (ImGui::IsItemToggledOpen())
                    {
                        if (open)
                            ActiveState->ExpandedBoneIndices.insert(Index);
                        else
                            ActiveState->ExpandedBoneIndices.erase(Index);
                    }

                    if (ImGui::IsItemClicked())
                    {
                        if (ActiveState->SelectedBoneIndex != Index)
                        {
                            ActiveState->SelectedBoneIndex = Index;
                            ActiveState->SelectedNotify.Invalidate();
                            ActiveState->bBoneLinesDirty = true;
                            ActiveState->bRequestScrollToBone = true;

                            ExpandToSelectedBone(ActiveState, Index);

                            if (ActiveState->PreviewActor && ActiveState->World)
                            {
                                ActiveState->PreviewActor->RepositionAnchorToBone(Index);
                                if (USceneComponent* Anchor = ActiveState->PreviewActor->GetBoneGizmoAnchor())
                                {
                                    ActiveState->World->GetSelectionManager()->SelectActor(ActiveState->PreviewActor);
                                    ActiveState->World->GetSelectionManager()->SelectComponent(Anchor);
                                }
                            }
                        }
                    }

                    if (!bLeaf && open)
                    {
                        for (int32 Child : Children[Index])
                        {
                            DrawNode(Child);
                        }
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                };

            for (int32 i = 0; i < Bones.size(); ++i)
            {
                if (Bones[i].ParentIndex < 0)
                {
                    DrawNode(i);
                }
            }
            ImGui::PopStyleVar();
            ImGui::EndChild();
        }
    }
}

void SViewerWindow::RenderRightPanel()
{
    if (!ActiveState)   return;

    ImGuiStyle& style = ImGui::GetStyle();
    
    // Panel header
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.30f, 0.30f, 0.30f, 0.8f));
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::Text("BONE PROPERTIES");
    ImGui::PopFont();
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 6));
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.35f, 0.35f, 0.35f, 0.6f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 6));
    ImGui::Spacing();

    // === 선택된 본의 트랜스폼 편집 UI ===
    if (ActiveState->SelectedBoneIndex >= 0 && ActiveState->CurrentMesh)
    {
        const FSkeleton* Skeleton = ActiveState->CurrentMesh->GetSkeleton();
        if (Skeleton && ActiveState->SelectedBoneIndex < Skeleton->Bones.size())
        {
            const FBone& SelectedBone = Skeleton->Bones[ActiveState->SelectedBoneIndex];

            // Selected bone header with icon-like prefix
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.90f, 0.40f, 1.0f));
            ImGui::Text("> Selected Bone");
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.95f, 1.00f, 1.0f));
            ImGui::TextWrapped("%s", SelectedBone.Name.c_str());
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.45f, 0.55f, 0.70f, 0.8f));
            ImGui::Separator();
            ImGui::PopStyleColor();

            // 본의 현재 트랜스폼 가져오기 (편집 중이 아닐 때만)
            if (!ActiveState->bBoneRotationEditing)
            {
                UpdateBoneTransformFromSkeleton(ActiveState);
            }

            ImGui::Spacing();

            // Location 편집
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
            ImGui::Text("Location");
            ImGui::PopStyleColor();

            ImGui::PushItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.28f, 0.20f, 0.20f, 0.6f));
            bool bLocationChanged = false;
            bLocationChanged |= ImGui::DragFloat("##BoneLocX", &ActiveState->EditBoneLocation.X, 0.1f, 0.0f, 0.0f, "X: %.3f");
            bLocationChanged |= ImGui::DragFloat("##BoneLocY", &ActiveState->EditBoneLocation.Y, 0.1f, 0.0f, 0.0f, "Y: %.3f");
            bLocationChanged |= ImGui::DragFloat("##BoneLocZ", &ActiveState->EditBoneLocation.Z, 0.1f, 0.0f, 0.0f, "Z: %.3f");
            ImGui::PopStyleColor();
            ImGui::PopItemWidth();

            if (bLocationChanged)
            {
                ApplyBoneTransform(ActiveState);
                ActiveState->bBoneLinesDirty = true;
            }

            ImGui::Spacing();

            // Rotation 편집
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
            ImGui::Text("Rotation");
            ImGui::PopStyleColor();

            ImGui::PushItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.20f, 0.28f, 0.20f, 0.6f));
            bool bRotationChanged = false;

            if (ImGui::IsAnyItemActive())
            {
                ActiveState->bBoneRotationEditing = true;
            }

            bRotationChanged |= ImGui::DragFloat("##BoneRotX", &ActiveState->EditBoneRotation.X, 0.5f, -180.0f, 180.0f, "X: %.2f°");
            bRotationChanged |= ImGui::DragFloat("##BoneRotY", &ActiveState->EditBoneRotation.Y, 0.5f, -180.0f, 180.0f, "Y: %.2f°");
            bRotationChanged |= ImGui::DragFloat("##BoneRotZ", &ActiveState->EditBoneRotation.Z, 0.5f, -180.0f, 180.0f, "Z: %.2f°");
            ImGui::PopStyleColor();
            ImGui::PopItemWidth();

            if (!ImGui::IsAnyItemActive())
            {
                ActiveState->bBoneRotationEditing = false;
            }

            if (bRotationChanged)
            {
                ApplyBoneTransform(ActiveState);
                ActiveState->bBoneLinesDirty = true;
            }

            ImGui::Spacing();

            // Scale 편집
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 1.0f, 1.0f));
            ImGui::Text("Scale");
            ImGui::PopStyleColor();

            ImGui::PushItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.20f, 0.20f, 0.28f, 0.6f));
            bool bScaleChanged = false;
            bScaleChanged |= ImGui::DragFloat("##BoneScaleX", &ActiveState->EditBoneScale.X, 0.01f, 0.001f, 100.0f, "X: %.3f");
            bScaleChanged |= ImGui::DragFloat("##BoneScaleY", &ActiveState->EditBoneScale.Y, 0.01f, 0.001f, 100.0f, "Y: %.3f");
            bScaleChanged |= ImGui::DragFloat("##BoneScaleZ", &ActiveState->EditBoneScale.Z, 0.01f, 0.001f, 100.0f, "Z: %.3f");
            ImGui::PopStyleColor();
            ImGui::PopItemWidth();

            if (bScaleChanged)
            {
                ApplyBoneTransform(ActiveState);
                ActiveState->bBoneLinesDirty = true;
            }
        }
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("Select a bone from the hierarchy to edit its transform properties.");
        ImGui::PopStyleColor();
    }
}

void SViewerWindow::UpdateBoneTransformFromSkeleton(ViewerState* State)
{
    if (!State || !State->CurrentMesh || State->SelectedBoneIndex < 0)
        return;

    // 본의 로컬 트랜스폼에서 값 추출
    const FTransform& BoneTransform = State->PreviewActor->GetSkeletalMeshComponent()->GetBoneLocalTransform(State->SelectedBoneIndex);
    State->EditBoneLocation = BoneTransform.Translation;
    State->EditBoneRotation = BoneTransform.Rotation.ToEulerZYXDeg();
    State->EditBoneScale = BoneTransform.Scale3D;
}

void SViewerWindow::ApplyBoneTransform(ViewerState* State)
{
    if (!State || !State->CurrentMesh || State->SelectedBoneIndex < 0)
        return;

    FTransform NewTransform(State->EditBoneLocation, FQuat::MakeFromEulerZYX(State->EditBoneRotation), State->EditBoneScale);
    State->PreviewActor->GetSkeletalMeshComponent()->SetBoneLocalTransform(State->SelectedBoneIndex, NewTransform);
}

void SViewerWindow::ExpandToSelectedBone(ViewerState* State, int32 BoneIndex)
{
    if (!State || !State->CurrentMesh)
        return;

    const FSkeleton* Skeleton = State->CurrentMesh->GetSkeleton();
    if (!Skeleton || BoneIndex < 0 || BoneIndex >= Skeleton->Bones.size())
        return;

    // 선택된 본부터 루트까지 모든 부모를 펼침
    int32 CurrentIndex = BoneIndex;
    while (CurrentIndex >= 0)
    {
        State->ExpandedBoneIndices.insert(CurrentIndex);
        CurrentIndex = Skeleton->Bones[CurrentIndex].ParentIndex;
    }
}

// ========================================
// 뷰어 툴바 관련 메서드 구현
// ========================================

void SViewerWindow::LoadViewerToolbarIcons(ID3D11Device* Device)
{
    // 기즈모 아이콘 로드
    IconSelect = NewObject<UTexture>();
    IconSelect->Load(GDataDir + "/Icon/Viewport_Toolbar_Select.png", Device);

    IconMove = NewObject<UTexture>();
    IconMove->Load(GDataDir + "/Icon/Viewport_Toolbar_Move.png", Device);

    IconRotate = NewObject<UTexture>();
    IconRotate->Load(GDataDir + "/Icon/Viewport_Toolbar_Rotate.png", Device);

    IconScale = NewObject<UTexture>();
    IconScale->Load(GDataDir + "/Icon/Viewport_Toolbar_Scale.png", Device);

    IconWorldSpace = NewObject<UTexture>();
    IconWorldSpace->Load(GDataDir + "/Icon/Viewport_Toolbar_WorldSpace.png", Device);

    IconLocalSpace = NewObject<UTexture>();
    IconLocalSpace->Load(GDataDir + "/Icon/Viewport_Toolbar_LocalSpace.png", Device);
}

AGizmoActor* SViewerWindow::GetGizmoActor()
{
    if (ActiveState && ActiveState->World)
    {
        return ActiveState->World->GetGizmoActor();
    }
    return nullptr;
}

void SViewerWindow::RenderViewerGizmoButtons()
{
    const ImVec2 IconSize(17, 17);

    // GizmoActor에서 직접 현재 모드 가져오기
    EGizmoMode CurrentGizmoMode = EGizmoMode::Select;
    AGizmoActor* GizmoActor = GetGizmoActor();
    if (GizmoActor)
    {
        CurrentGizmoMode = GizmoActor->GetMode();
    }

    // Select 버튼 (Q)
    bool bIsSelectActive = (CurrentGizmoMode == EGizmoMode::Select);
    ImVec4 SelectTintColor = bIsSelectActive ? ImVec4(0.3f, 0.6f, 1.0f, 1.0f) : ImVec4(1, 1, 1, 1);

    if (IconSelect && IconSelect->GetShaderResourceView())
    {
        if (ImGui::ImageButton("##SelectBtn", (void*)IconSelect->GetShaderResourceView(), IconSize,
            ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), SelectTintColor))
        {
            if (GizmoActor)
            {
                GizmoActor->SetMode(EGizmoMode::Select);
            }
        }
    }
    else
    {
        if (ImGui::Button("Select", ImVec2(60, 0)))
        {
            if (GizmoActor)
            {
                GizmoActor->SetMode(EGizmoMode::Select);
            }
        }
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("오브젝트를 선택합니다. [Q]");
    }
    ImGui::SameLine();

    // Move 버튼 (W)
    bool bIsMoveActive = (CurrentGizmoMode == EGizmoMode::Translate);
    ImVec4 MoveTintColor = bIsMoveActive ? ImVec4(0.3f, 0.6f, 1.0f, 1.0f) : ImVec4(1, 1, 1, 1);

    if (IconMove && IconMove->GetShaderResourceView())
    {
        if (ImGui::ImageButton("##MoveBtn", (void*)IconMove->GetShaderResourceView(), IconSize,
            ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), MoveTintColor))
        {
            if (GizmoActor)
            {
                GizmoActor->SetMode(EGizmoMode::Translate);
            }
        }
    }
    else
    {
        if (ImGui::Button("Move", ImVec2(60, 0)))
        {
            if (GizmoActor)
            {
                GizmoActor->SetMode(EGizmoMode::Translate);
            }
        }
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("오브젝트를 선택하고 이동시킵니다. [W]");
    }
    ImGui::SameLine();

    // Rotate 버튼 (E)
    bool bIsRotateActive = (CurrentGizmoMode == EGizmoMode::Rotate);
    ImVec4 RotateTintColor = bIsRotateActive ? ImVec4(0.3f, 0.6f, 1.0f, 1.0f) : ImVec4(1, 1, 1, 1);

    if (IconRotate && IconRotate->GetShaderResourceView())
    {
        if (ImGui::ImageButton("##RotateBtn", (void*)IconRotate->GetShaderResourceView(), IconSize,
            ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), RotateTintColor))
        {
            if (GizmoActor)
            {
                GizmoActor->SetMode(EGizmoMode::Rotate);
            }
        }
    }
    else
    {
        if (ImGui::Button("Rotate", ImVec2(60, 0)))
        {
            if (GizmoActor)
            {
                GizmoActor->SetMode(EGizmoMode::Rotate);
            }
        }
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("오브젝트를 선택하고 회전시킵니다. [E]");
    }
    ImGui::SameLine();

    // Scale 버튼 (R)
    bool bIsScaleActive = (CurrentGizmoMode == EGizmoMode::Scale);
    ImVec4 ScaleTintColor = bIsScaleActive ? ImVec4(0.3f, 0.6f, 1.0f, 1.0f) : ImVec4(1, 1, 1, 1);

    if (IconScale && IconScale->GetShaderResourceView())
    {
        if (ImGui::ImageButton("##ScaleBtn", (void*)IconScale->GetShaderResourceView(), IconSize,
            ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ScaleTintColor))
        {
            if (GizmoActor)
            {
                GizmoActor->SetMode(EGizmoMode::Scale);
            }
        }
    }
    else
    {
        if (ImGui::Button("Scale", ImVec2(60, 0)))
        {
            if (GizmoActor)
            {
                GizmoActor->SetMode(EGizmoMode::Scale);
            }
        }
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("오브젝트를 선택하고 스케일을 조절합니다. [R]");
    }

    ImGui::SameLine();
}

void SViewerWindow::RenderViewerGizmoSpaceButton()
{
    const ImVec2 IconSize(17, 17);

    // GizmoActor에서 직접 현재 스페이스 가져오기
    EGizmoSpace CurrentGizmoSpace = EGizmoSpace::World;
    AGizmoActor* GizmoActor = GetGizmoActor();
    if (GizmoActor)
    {
        CurrentGizmoSpace = GizmoActor->GetSpace();
    }

    // 현재 스페이스에 따라 적절한 아이콘 표시
    bool bIsWorldSpace = (CurrentGizmoSpace == EGizmoSpace::World);
    UTexture* CurrentIcon = bIsWorldSpace ? IconWorldSpace : IconLocalSpace;
    const char* TooltipText = bIsWorldSpace ? "월드 스페이스 좌표 [Tab]" : "로컬 스페이스 좌표 [Tab]";

    // 선택 상태 tint (월드/로컬 모두 동일하게 흰색)
    ImVec4 TintColor = ImVec4(1, 1, 1, 1);

    if (CurrentIcon && CurrentIcon->GetShaderResourceView())
    {
        if (ImGui::ImageButton("##GizmoSpaceBtn", (void*)CurrentIcon->GetShaderResourceView(), IconSize,
            ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), TintColor))
        {
            // 버튼 클릭 시 스페이스 전환
            if (GizmoActor)
            {
                EGizmoSpace NewSpace = bIsWorldSpace ? EGizmoSpace::Local : EGizmoSpace::World;
                GizmoActor->SetSpace(NewSpace);
            }
        }
    }
    else
    {
        // 아이콘이 없는 경우 텍스트 버튼
        const char* ButtonText = bIsWorldSpace ? "World" : "Local";
        if (ImGui::Button(ButtonText, ImVec2(60, 0)))
        {
            if (GizmoActor)
            {
                EGizmoSpace NewSpace = bIsWorldSpace ? EGizmoSpace::Local : EGizmoSpace::World;
                GizmoActor->SetSpace(NewSpace);
            }
        }
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", TooltipText);
    }

    ImGui::SameLine();
}

void SViewerWindow::RenderViewerToolbar()
{
    // 툴바 높이
    const float ToolbarHeight = 35.0f;

    // 툴바 영역 시작 (탭 키 네비게이션 완전 비활성화)
    ImGui::BeginChild("ViewerToolbar", ImVec2(0, ToolbarHeight), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus);

    // 상단 여백 추가 (메인 뷰처럼)
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);

    // 기즈모 버튼 스타일 설정 (메인 뷰와 동일)
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));      // 간격 좁히기
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);            // 모서리 둥글게
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));        // 배경 투명
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.5f)); // 호버 배경

    // 기즈모 모드 버튼 (Q/W/E/R)
    RenderViewerGizmoButtons();

    // 구분선
    ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "|");
    ImGui::SameLine();

    // 월드/로컬 스페이스 토글
    RenderViewerGizmoSpaceButton();

    // 기즈모 버튼 스타일 복원
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    ImGui::EndChild();
}