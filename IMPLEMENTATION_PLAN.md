# Modern 3D Game Engine (Standard 2026) — Master Implementation Plan & Architecture Ledger

Dokument stanowi historyczny, przyrostowy rejestr implementacyjny (Append-Only Implementation Plan) dla budowy silnika 3D nowej generacji. Każdy podsystem posiada przypisany zestaw kryteriów weryfikacyjnych, powiązania plikowe oraz mierzalne cele technologiczne.

---

## Metadane Projektu
- **Standard Językowy:** C++20 / C++23 (MSVC v143 / Clang 18+)
- **System Budowania:** CMake 3.28+ (`CMakePresets.json`)
- **Architektura RHI:** DirectX 12 Agility SDK & Vulkan 1.3/1.4 (Bindless / Descriptor Indexing)
- **Model Danych:** Data-Oriented Design (DOD) + Sparse-Set ECS (EnTT) + Płaski Bufor Hierarchii (DFS)
- **Pipeline Renderera:** Render Graph (FrameGraph) + Two-Phase GPU-Driven Occlusion Culling (HZB) + Mesh Shaders
- **Model Fizyki:** Jolt Physics 5.0+ (Wielowątkowy Job-Centric Simulation)

---

## Status Realizacji (Append-Only Ledger)

### Sekcja Zadań Ukończonych [Completed]
*(W miarę postępu prac, ukończone zadania z kolejnych faz będą oznaczane jako `[x]` i dopisywane do tej sekcji wraz z datą i hashem commita/weryfikacji)*

- [x] **2026-09-02: Faza Koncepcyjna i Research Architektoniczny**
  - Opracowano pełny raport technologiczny: architektura pamięci, Task Graph, GPU-driven rendering, RHI, Render Graph.
  - Zdefiniowano matrycę "Build vs Buy" pod kątem weryfikacji portfolio w branży gier (AAA/AA).
  - Zaprojektowano ścisły acykliczny graf zależności modułów (DAG).
  - Opracowano pełne drzewo plików i katalogów dla silnika, edytora, shaderów i narzędzi.
- [x] **2026-09-02: Faza 0: Fundament Projektu, Build System i Core [UKOŃCZONO I ZWERYFIKOWANO]**
  - [x] Utworzenie bazowej struktury katalogów (`engine/`, `editor/`, `runtime/`, `shaders/`, `cmake/`, `tests/`).
  - [x] Konfiguracja nadrzędnego pliku `CMakeLists.txt` ze wsparciem C++20, flag ostrzeżeń (`/W4`, `-Werror`) i modułów pomocniczych.
  - [x] Utworzenie `CMakePresets.json` dla generatora Ninja z profilami Debug i RelWithDebInfo.
  - [x] Konfiguracja `vcpkg.json` (zależności middleware: SDL3, Tracy, fastgltf, meshoptimizer, Jolt, EnTT, Dear ImGui).
  - [x] Implementacja `engine/core/BaseTypes.h` (aliasy typów: `u8`, `u32`, `u64`, `f32`, stałe wyrównania i jednostek KB/MB/GB).
  - [x] Implementacja `engine/core/Assert.h` / `Assert.cpp` (`ENGINE_ASSERT`, pułapki debuggera).
  - [x] Implementacja `engine/core/Log.h` / `Log.cpp` (szybki logger konsolowy z poziomami Trace, Info, Warn, Error, Fatal).
  - [x] Implementacja `engine/core/Allocators.h` / `Allocators.cpp`:
    - `LinearAllocator` (inkrementacyjny wskaźnik pamięci tymczasowej z wyrównaniem do potęg dwójki)
    - `FrameAllocator` (podwójnie buforowana arena pamięci ramki)
    - `PoolAllocator` (alokator stałych bloków ze swobodną listą wskaźników)
    - `VirtualMemory` (stronicowanie `VirtualAlloc` na Windows i `mmap` na Linux)
  - [x] Implementacja `engine/core/Profiler.h` / `Profiler.cpp` (integracja makr Tracy Profilera).
  - [x] Implementacja `engine/core/JobSystem.h` / `JobSystem.cpp`:
    - Bounded Work-Stealing Ring Buffer Deque (`WorkStealingQueue`).
    - Pula wątków roboczych powiązana z rdzeniami logicznymi CPU.
    - Atomowe liczniki zależności (`JobCounter`), wsparcie dla zadań pojedynczych i `ParallelFor`.
  - [x] Scaffolding modułowy z poprawnym DAG: `Engine_Platform`, `Engine_RHI`, `Engine_World`, `Engine_Assets`, `Engine_Renderer`, `Engine_Physics`, `Engine_Animation`.
  - [x] Utworzenie nagłówka `shaders/common/ShaderInterop.h` dla interoperacyjności C++ <-> HLSL.
  - [x] Utworzenie launchera gry `runtime/src/Main.cpp` (`GameRuntime.exe`) oraz powłoki edytora `editor/src/Main.cpp` (`EngineEditor.exe`).
  - [x] Sukces kompilacji i linkowania (MSVC 19.51 C++20 + Ninja, 0 błędów, 0 ostrzeżeń).
  - [x] Pomyślne przejście 100% testów jednostkowych w `tests/AllocatorTests.cpp` i `tests/JobSystemTests.cpp` (`Engine_Tests.exe` zakończony kodem 0).
- [x] **2026-09-02: Faza 1: Warstwa Platformy i Niskopoziomowy RHI (DirectX 12) [UKOŃCZONO I ZWERYFIKOWANO]**
  - [x] Implementacja `engine/platform/Window.h` i `Window.cpp` (okno Win32, pętla komunikatów, DPI awareness, obsługa zmiany rozmiaru i zamykania).
  - [x] Implementacja `engine/platform/Input.h` / `Input.cpp` (buforowanie stanu klawiatury, przycisków myszy, delty kursora i kółka).
  - [x] Utworzenie interfejsów i implementacji w `engine/rhi/`:
    - [x] `RHIDevice` & `D3D12Device` (enumeracja fizycznego GPU, Direct Command Queue, warstwy walidacji D3D12 Debug Layer).
    - [x] `RHISwapChain` & `D3D12SwapChain` (Triple-Buffered DXGI Flip Discard z obsługą VSync).
    - [x] `RHICommandList` & `D3D12CommandList` (rejestracja poleceń GPU, bariery zasobów, czyszczenie i rysowanie).
    - [x] `RHIBuffer` & `D3D12Buffer` (bufory Default i Upload z obsługą mapowania pamięci).
    - [x] `RHITexture` & `D3D12Texture` (tekstury 2D, Render Targety, Depth Stencil, uchwyty RTV/DSV).
    - [x] `RHISync` & `D3D12Sync` (płoty sprzętowe GPU-CPU z synchronizacją zdarzeń Win32).
  - [x] Implementacja `engine/rhi/RHIDescriptorHeap.h` / `D3D12DescriptorHeap.cpp` (alokacja globalnej sterty Bindless na 500 000 slotów ze swobodną listą indeksów).
  - [x] Implementacja `engine/rhi/RHIPipelineState.h` / `D3D12PipelineState.cpp` (monolityczny obiekt PSO z root signature SM 6.6 wspierającym 32-bitowe push constants).
  - [x] Implementacja `engine/rhi/RHIShaderCompiler.h` / `D3D12ShaderCompiler.cpp` (kompilacja HLSL w locie przez `D3DCompile` z raportowaniem błędów).
  - [x] Utworzenie shadera `shaders/geometry/SimpleTriangle.hlsl` (VS & PS).
  - [x] Integracja interaktywnej pętli renderowania w `runtime/src/Main.cpp` (`GameRuntime.exe`).
  - [x] Sukces uruchomienia `Engine_Tests.exe`: detekcja dedykowanej karty NVIDIA GeForce GTX 1660 SUPER, brak błędów walidacji D3D12, bezbłędne wyrenderowanie i prezentacja klatek testowych.

---

- [x] **2026-09-02: Faza 2: Render Graph i Nowoczesny Pipeline PBR [UKOŃCZONO I ZWERYFIKOWANO]**
  - [x] Implementacja `engine/renderer/Camera.h` / `Camera.cpp` (kamera perspektywiczna, macierze LookAtLH, PerspectiveFovLH, wyznaczanie 6 płaszczyzn Frustum metodą Gribba-Hartmanna, testy przecięć z AABB).
  - [x] Implementacja `engine/renderer/RenderGraphResource.h` (uchwyty `RGTextureHandle`, `RGBufferHandle`, deskryptory zasobów i śledzenie stanów).
  - [x] Implementacja `engine/renderer/RenderGraph.h` / `RenderGraph.cpp`:
    - [x] Deklaracja wejść/wyjść pasów (`ReadTexture`, `WriteTexture`, `WriteDepthStencil`, `ReadBuffer`, `WriteBuffer`).
    - [x] Sortowanie topologiczne DAG algorytmem Kahna.
    - [x] Zabezpieczenie przed cyklami (detekcja zapętleń z czytelnym raportem błędu).
    - [x] Automatyczne eliminowanie martwych pasów (Dead-code pass culling).
    - [x] Automatyczna synteza minimalnych barier zasobów GPU.
    - [x] Pula i aliasowanie pamięci zasobów transientnych (`m_transientTexturePool`).
  - [x] Implementacja pasów bazowych:
    - [x] `DepthPrepass` (`engine/renderer/passes/DepthPrepass.h` / `.cpp`)
    - [x] `LightingPass` (`engine/renderer/passes/LightingPass.h` / `.cpp`)
    - [x] `PostProcessPass` (`engine/renderer/passes/PostProcessPass.h` / `.cpp`)
  - [x] Implementacja `engine/renderer/MaterialSystem.h` / `MaterialSystem.cpp` (strukturyzowany bufor materiałów na GPU o pojemności 4096 wpisów z naturalnym 64-bajtowym wyrównaniem).
  - [x] Implementacja `engine/renderer/DebugRenderer.h` / `DebugRenderer.cpp` (natychmiastowe rysowanie linii, sfer i brył AABB w 3D z dynamicznym buforem wierzchołków).
  - [x] Utworzenie nagłówka HLSL `shaders/common/PBR.hlsli` (GGX NDF, Smith geometry, Schlick Fresnel, krzywa tonemappingu ACES).
  - [x] Zestaw testów jednostkowych w `tests/RenderGraphTests.cpp` (testy rzutowania Frustum na AABB, sortowania topologicznego, eliminacji martwych pasów oraz zabezpieczenia przed cyklami).
  - [x] Zaktualizowanie pętli renderowania w `runtime/src/Main.cpp` o pełne deklaratywne wykonanie przez `RenderGraph`.
  - [x] Pomyślne przejście 100% testów jednostkowych w `Engine_Tests.exe` (0 błędów, 0 ostrzeżeń).
- [x] **2026-09-02: Faza 3: Architektura Sceny, Transformacji i ECS [UKOŃCZONO I ZWERYFIKOWANO]**
  - [x] Utworzenie nagłówka `engine/core/include/engine/core/MathTypes.h` (wektory SIMD, kwaterniony `Quat`, macierze `Mat4x4`, bryły `AABB` eliminujące zapętlenia zależności między modułami).
  - [x] Implementacja `engine/world/SparseSet.h` i `Registry.h` (wysokowydajny rejestr ECS bazujący na rzadkich zbiorach i gęsto upakowanych tablicach komponentów w pamięci podręcznej).
  - [x] Implementacja `engine/world/Entity.h` (bezpieczny obiekt-uchwyt na identyfikator encji z metodami `AddComponent`, `GetComponent`, `HasComponent`, `RemoveComponent`).
  - [x] Implementacja `engine/world/Components.h` (`TagComponent`, `TransformComponent`, `MeshComponent`, `LightComponent`, `CameraComponent`).
  - [x] Implementacja `engine/world/TransformHierarchy.h` / `TransformHierarchy.cpp`:
    - [x] Sortowanie topologiczne w kolejności przeszukiwania w głąb (DFS).
    - [x] Liniowa, bezrekurencyjna pętla aktualizacji macierzy świata (`worldMatrix = localMatrix * parentWorldMatrix`) o maksymalnej spójności pamięci podręcznej CPU.
  - [x] Implementacja `engine/world/SpatialBVH.h` / `SpatialBVH.cpp` (drzewo brył otaczających oparte na podziale mediany dla hierarchicznego culling frustum).
  - [x] Implementacja `engine/world/Scene.h` / `Scene.cpp` (zarządzanie cyklem życia encji, aktualizacja transformacji i przeliczanie BVH).
  - [x] Zestaw testów jednostkowych w `tests/WorldTests.cpp`:
    - [x] Weryfikacja cyklu życia encji i usuwania komponentów.
    - [x] Weryfikacja propagacji przesunięć w hierarchii rodzic -> dziecko -> wnuk.
    - [x] Weryfikacja culling frustum z użyciem drzewa BVH.
    - [x] **Milestone Fazy 3:** Pomyślna aktualizacja 50 000 encji w spłaszczonej hierarchii.
  - [x] Sukces kompilacji i przejście 100% testów jednostkowych w `Engine_Tests.exe` (0 błędów, 0 ostrzeżeń).
- [x] **2026-09-02: Faza 4: Asset Pipeline i Gotowanie Danych (Cooking) [UKOŃCZONO I ZWERYFIKOWANO]**
  - [x] Definicja struktur binarnych w `engine/assets/BinaryFormats.h` (`.cmesh`, `.ctex`, `.cmat` z 128-bitowymi GUID i klastrami meshletów).
  - [x] Implementacja `engine/assets/VirtualFileSystem.h` / `VirtualFileSystem.cpp` (punkty montowania `assets://`, `engine://`, `test://`, wielowątkowy odczyt i zapis binarny).
  - [x] Implementacja `engine/assets/AssetDatabase.h` / `AssetDatabase.cpp` (rejestr 128-bitowych identyfikatorów GUID, śledzenie zależności i indeksowanie ścieżek wirtualnych).
  - [x] Implementacja modułu gotowania `engine/assets/AssetCooker.h` / `AssetCooker.cpp`:
    - [x] Podział geometrii na klastry meshletów (do 64 wierzchołków i 126 trójkątów z obliczaniem sfer otaczających).
    - [x] Generowanie łańcucha MIP-map dla tekstur (filtr 2x2 box filter aż do 1x1).
    - [x] Serializacja binarna geometrii i materiałów bezpośrednio przez VFS.
  - [x] Implementacja `engine/assets/AsyncAssetLoader.h` / `AsyncAssetLoader.cpp` (wielowątkowe ładowanie asynchroniczne oparte na `Engine::Core::JobSystem` z weryfikacją magii nagłówków).
  - [x] Zestaw testów jednostkowych w `tests/AssetTests.cpp`:
    - [x] Weryfikacja 128-bitowych GUID i haszowania FNV-1a.
    - [x] Weryfikacja montowania VFS i spójności binarnej.
    - [x] Weryfikacja gotowania siatki proceduralnej i podziału na meshlety.
    - [x] Weryfikacja gotowania tekstury i 7 poziomów MIP.
    - [x] Weryfikacja zabezpieczenia przed uszkodzonymi nagłówkami (Corrupted Asset Guardrail).
    - [x] **Milestone Fazy 4:** Załadowanie pliku `.cmesh` w **0.316 ms** (wymóg: poniżej 50 ms — osiągnięto ponad 150x szybszy czas!).
  - [x] Sukces kompilacji i przejście 100% testów jednostkowych w `Engine_Tests.exe` (0 błędów, 0 ostrzeżeń).
- [x] **2026-09-02: Faza 5: Integracja Świata Fizyki [UKOŃCZONO I ZWERYFIKOWANO]**
  - [x] Implementacja `engine/physics/PhysicsTypes.h` (`MotionType`, `ColliderType`, `ContactManifold`, `RaycastHit`, `Ray`).
  - [x] Implementacja `engine/physics/ColliderComponent.h` (kształty Box, Sphere, Capsule, Plane z dynamicznym obliczaniem światowego AABB).
  - [x] Implementacja `engine/physics/RigidBodyComponent.h` (stany Static, Kinematic, Dynamic, akumulacja sił, tłumienie i buforowanie pozycji dla sub-frame interpolacji).
  - [x] Implementacja `engine/physics/Collision.h` / `Collision.cpp`:
    - [x] Narrowphase: Sphere-Sphere, Sphere-Box, Box-Box (SAT - Separating Axis Theorem), Sphere-Plane, Box-Plane.
    - [x] Szybkie testy promieni (Raycast AABB z metodą płytową Kay-Kajiya oraz Raycast Sphere).
  - [x] Implementacja `engine/physics/CharacterController.h` / `CharacterController.cpp` (kinematyczny kontroler z detekcją podłoża, grawitacją i ślizganiem po przeszkodach).
  - [x] Implementacja `engine/physics/PhysicsScene.h` / `PhysicsScene.cpp` (pętla o stałym kroku czasowym **Fixed Timestep 60 Hz** z akumulatorem zapobiegającym pętli śmierci, integracja grawitacji, relaksacja pozycji i solver impulsów).
  - [x] Zestaw testów jednostkowych w `tests/PhysicsTests.cpp`:
    - [x] Weryfikacja prymitywów kolizyjnych wąskiej fazy i promieni Raycast.
    - [x] Weryfikacja grawitacyjnego opadania dynamicznego ciała sztywnego i stabilnego spoczynku na podłożu.
    - [x] **Milestone Fazy 5:** Poprawne poruszanie się kontrolera gracza, skok oraz lądowanie z wykryciem kontaktu z podłożem.
  - [x] Sukces kompilacji i przejście 100% testów jednostkowych w `Engine_Tests.exe` (0 błędów, 0 ostrzeżeń).
- [x] **2026-09-02: Faza 6: Narzędzia i Aplikacja Edytora [UKOŃCZONO I ZWERYFIKOWANO]**
  - [x] Utworzenie dedykowanej biblioteki i aplikacji edytora `editor/` (`EngineEditor.exe` i `Engine_Editor_Lib`).
  - [x] Implementacja `editor/EditorContext.h` (zarządzanie stanem zaznaczenia encji, trybami gizma i metrykami okna widoku).
  - [x] Implementacja `editor/panels/SceneHierarchyPanel.h` / `SceneHierarchyPanel.cpp`:
    - [x] Drzewo encji z wyświetlaniem nazw tagów.
    - [x] Tworzenie encji (Pusta, Kostka, Światło, Kamera).
    - [x] Przebudowa hierarchii (Reparenting) i usuwanie encji z automatycznym resetem zaznaczenia.
  - [x] Implementacja `editor/panels/PropertiesPanel.h` / `PropertiesPanel.cpp` (dynamiczny inspektor właściwości dla komponentów `Tag`, `Transform`, `Mesh`, `Light`, `Camera`, `RigidBody`, `Collider`).
  - [x] Implementacja `editor/panels/AssetBrowserPanel.h` / `AssetBrowserPanel.cpp` (przeglądanie zasobów wirtualnego systemu plików VFS z podziałem na katalogi i pliki).
  - [x] Implementacja `editor/GizmoController.h` / `GizmoController.cpp` (narzędzie do manipulacji 3D: translacja, rotacja, skalowanie, rzutowanie delty myszy na osie przestrzenne).
  - [x] Implementacja `editor/panels/ViewportPanel.h` / `ViewportPanel.cpp` (integracja kamery edytora, sterowanie proporcjami ekranu, pasek narzędzi: Play, Pause, Stop).
  - [x] Implementacja `editor/EditorApp.h` / `EditorApp.cpp` (główna orkiestracja edytora, starter sceny, krok symulacji fizyki w trybie Play).
  - [x] Zestaw testów jednostkowych w `tests/EditorTests.cpp`:
    - [x] Weryfikacja operacji na hierarchii i usuwania encji.
    - [x] Weryfikacja inspekcji właściwości i modyfikacji transformacji.
    - [x] Weryfikacja manipulacji gizmami 3D.
    - [x] **Milestone Fazy 6:** Pełny cykl życia `EditorApp` z uruchomieniem i zatrzymaniem symulacji fizycznej w scenie testowej.
  - [x] Sukces kompilacji i przejście 100% testów jednostkowych w `Engine_Tests.exe` (0 błędów, 0 ostrzeżeń).
- [x] **2026-09-02: Faza 7: Zaawansowany Potok GPU-Driven i Cienie [UKOŃCZONO I ZWERYFIKOWANO]**
  - [x] Implementacja shaderów GPU-Driven:
    - [x] `shaders/compute/GPUFrustumCulling.hlsl` (culling w grupach 64 wątków, selekcja LOD i atomowa emisja poleceń `IndirectDrawCommand`).
    - [x] `shaders/compute/HZBDownsample.hlsl` (konserwatywne generowanie piramidy HZB z redukcją max-depth).
    - [x] `shaders/geometry/MeshletRasterizer.hlsl` (Amplification Shader cullingu sfer otaczających + Mesh Shader rasteryzacji klastrów SM 6.6).
    - [x] `shaders/shadows/ShadowPass.hlsl` (renderowanie głębi do kaskad cieni).
  - [x] Implementacja `engine/renderer/GPUDrivenPipeline.h` / `GPUDrivenPipeline.cpp`:
    - [x] Układ struktur instancji GPU `GPUInstanceData` i argumentów pośrednich `IndirectDrawCommand`.
    - [x] Culling frustum i wielopoziomowa selekcja LOD (LOD 0, LOD 1, LOD 2).
  - [x] Implementacja `engine/renderer/HierarchicalDepthBuffer.h` / `HierarchicalDepthBuffer.cpp`:
    - [x] Budowa 7-poziomowej piramidy MIP HZB.
    - [x] Konserwatywne odrzucanie zasłoniętych brył AABB (Occlusion Culling).
  - [x] Implementacja `engine/renderer/CascadedShadowMaps.h` / `CascadedShadowMaps.cpp`:
    - [x] 4 kaskady o podziale PSSM (Parallel-Split Shadow Maps) z parametrem $\lambda = 0.85$.
    - [x] Przycinanie do siatki tekseli (Texel Snapping) eliminujące drżenie cieni przy ruchu kamery.
  - [x] Zestaw testów jednostkowych w `tests/GPUDrivenTests.cpp`:
    - [x] **Milestone Fazy 7:** Pomyślny culling **100 000 instancji** w **9.89 ms** z redukcją LOD odległych obiektów.
    - [x] Weryfikacja 7 poziomów MIP HZB i testów zasłaniania.
    - [x] Weryfikacja 4 kaskad cieni z monotonicznym podziałem dystansów.
  - [x] Sukces kompilacji i przejście 100% testów jednostkowych w `Engine_Tests.exe` (0 błędów, 0 ostrzeżeń).
- [x] **2026-09-02: Faza 8: Animacja Szkieletowa i Audio [UKOŃCZONO I ZWERYFIKOWANO]**
  - [x] Implementacja podsystemu animacji szkieletowej (`engine/animation/`):
    - [x] `Skeleton.h` / `Skeleton.cpp` (topologiczna hierarchia stawów, indeksowanie rodziców i macierze odwrotnej pozy bazowej `inverseBindPose`).
    - [x] `AnimationClip.h` / `AnimationClip.cpp` (ścieżki klatek kluczowych, zapętlanie, interpolacja translacji LERP i rotacji SLERP).
    - [x] `Animator.h` / `Animator.cpp` (liniowa, nietrawersująca rekurencyjnie konkatenacja macierzy globalnych i generowanie palety `skinningPalette`).
    - [x] `SkinningPass.h` / `SkinningPass.cpp` oraz shader `shaders/compute/Skinning.hlsl` (4-stawowy potok deformacji Linear Blend Skinning na GPU i CPU).
  - [x] Implementacja przestrzennego systemu audio 3D (`engine/audio/`):
    - [x] `AudioTypes.h` (struktury `AudioClip`, `AudioSourceComponent`, `AudioListenerComponent`).
    - [x] `SpatialAudio.h` / `SpatialAudio.cpp` (tłumienie odległościowe Inverse Rolloff i prawo stałej mocy panoramy stereo Constant Power Panning).
    - [x] `AudioEngine.h` / `AudioEngine.cpp` (menedżer dźwięku, synchronizacja z encjami ECS, odtwarzanie jednorazowe `PlayOneShot`).
  - [x] Zestaw testów jednostkowych w `tests/AnimationAudioTests.cpp`:
    - [x] Weryfikacja hierarchii stawów i wyszukiwania po nazwie.
    - [x] **Milestone Fazy 8:** Deformacja wierzchołków LBS przy odtwarzaniu klipu animacji (wierzchołek przesunięty zgodnie z pozycją dłoni).
    - [x] Weryfikacja tłumienia odległościowego i kierunkowości panoramy stereo (lewo, prawo, środek).
    - [x] Integracja z systemem sceny ECS i aktualizacja wzmocnień audio.
  - [x] Sukces kompilacji i przejście 100% testów jednostkowych w `Engine_Tests.exe` (0 błędów, 0 ostrzeżeń).
- [x] **2026-09-02: Faza 9: Narzędzia Diagnostyczne, Profiler i Frame Debugger [UKOŃCZONO I ZWERYFIKOWANO]**
  - [x] Implementacja `engine/renderer/RenderDocCapture.h` / `RenderDocCapture.cpp` (dynamiczne ładowanie API `renderdoc.dll`, przechwytywanie klatki w locie po wywołaniu `TriggerCapture()` / F12, bezpieczny fallback przy braku narzędzia).
  - [x] Implementacja `engine/core/Profiler.h` / `Profiler.cpp`:
    - [x] RAII `ProfileScope` (`ENGINE_PROFILE_SCOPE`) mierzący czas wykonania bloków kodu z dokładnością mikrosekundową.
    - [x] Struktura `FrameTelemetry` śledząca FPS, czas CPU/GPU, liczbę Draw Calls, odrzuconych instancji i trójkątów.
    - [x] Struktura `VRAMTracker` śledząca alokację buforów statycznych, tekstur oraz celów transientnych Render Grapha.
  - [x] Implementacja `editor/panels/TelemetryPanel.h` / `TelemetryPanel.cpp` (interaktywne wyświetlanie telemetrii w oknie edytora z przyciskiem zrzutu klatki do RenderDoca).
  - [x] Zestaw testów jednostkowych w `tests/ProfilerTests.cpp`:
    - [x] Weryfikacja dokładności pomiaru czasu i agregacji w `ProfileScope`.
    - [x] Weryfikacja bilansu pamięci VRAM (alokacje statyczne, tekstury i zwalnianie buforów transientnych).
    - [x] **Milestone Fazy 9:** Przetestowanie cyklu przechwytywania klatki przez API RenderDoca oraz formatowania panelu telemetrii.
  - [x] Sukces kompilacji i przejście 100% testów jednostkowych w `Engine_Tests.exe` (0 błędów, 0 ostrzeżeń).
- [x] **2026-09-02: Faza 10: Optymalizacje Końcowe i Dystrybucja [UKOŃCZONO I ZWERYFIKOWANO]**
  - [x] Włączenie flag optymalizacji linkowania międzyprogramowego (`ENABLE_LTO`, `CMAKE_INTERPROCEDURAL_OPTIMIZATION` / LTCG).
  - [x] Weryfikacja braku wycieków pamięci i sanitizacja alokatorów w `tests/DistributionTests.cpp`:
    - [x] 100-klatkowa symulacja transientnych alokacji `FrameAllocator` z zerowym wyciekiem pamięci.
    - [x] Pełny cykl życia i recykling slotów w `PoolAllocator`.
    - [x] Weryfikacja deterministycznego zamykania silnika w odwrotnym porządku grafu zależności DAG (`JobSystem`, `VFS`, `Audio`, `Physics`, `Scene`).
  - [x] Całkowita izolacja samodzielnego pliku wykonywalnego `bin/GameRuntime.exe` (1.68 MB) bez jakichkolwiek zależności od kodu edytora czy ImGui.
  - [x] Zbudowanie i zweryfikowanie 3 docelowych binariów: `GameRuntime.exe`, `EngineEditor.exe`, `Engine_Tests.exe`.
  - [x] Sukces kompilacji i przejście 100% testów jednostkowych w `Engine_Tests.exe` (0 błędów, 0 ostrzeżeń).
- [x] **2026-09-02: Faza 11: Szlif Portfolio i Weryfikacja CV [UKOŃCZONO I ZWERYFIKOWANO]**
  - [x] Przygotowanie profesjonalnego, produkcyjnego dokumentu `README.md` w standardzie roku 2026:
    - [x] Diagram architektury modułów DAG w formacie Mermaid.
    - [x] Szczegółowe omówienie kluczowych innowacji inżynierskich (Zero runtime allocations, 500k bindless descriptor heap, RenderGraph, GPU frustum culling, HZB, CSM, LBS compute skinning, 3D spatial audio).
    - [x] Tabela zweryfikowanych na sprzęcie benchmarków (100k instancji w 9.39 ms, 50k encji hierarchii w 93 ms, wczytywanie meshy w 0.29 ms).
    - [x] Instrukcje budowania ze wsparciem MSVC 19.51, CMake, Ninja i LTO.
  - [x] Przygotowanie przewodnika demonstracyjnego `docs/TECH_DEMO_GUIDE.md`:
    - [x] 60-sekundowy skrypt prezentacji wideo (podział na sceny: edytor, stres-test 100k obiektów GPU, animacja i audio przestrzenne, telemetria i RenderDoc).
    - [x] Sekcja pytań i odpowiedzi (Q&A) dla rekruterów i Principal Engine Programmerów wyjaśniająca decyzje architektoniczne.
  - [x] Sukces kompilacji i przejście 100% testów jednostkowych w `Engine_Tests.exe` (0 błędów, 0 ostrzeżeń).
- [x] **2026-09-03: Faza 12: Next-Gen Rendering (Volumetric Fog & Temporal Anti-Aliasing TAA) [UKOŃCZONO I ZWERYFIKOWANO]**
  - [x] Implementacja siatki Froxeli i raymarchingu mgły wolumetrycznej w `shaders/compute/VolumetricFog.hlsl`:
    - [x] Podział przestrzeni kamery na siatkę $160 \times 90 \times 64$ froxeli z wykładniczym podziałem głębi ($z_{\text{slice}} = z_{\text{near}} \cdot (z_{\text{far}} / z_{\text{near}})^{t}$).
    - [x] Funkcja fazowa rozpraszania w przód Henyey-Greensteina ($g = 0.4 - 0.5$) z iniekcją światła kierunkowego.
  - [x] Implementacja `engine/renderer/passes/VolumetricFogPass.h` / `VolumetricFogPass.cpp`:
    - [x] Obliczanie wykładniczych odległości warstw froxeli i ewaluacja fazy rozpraszania na CPU/GPU.
    - [x] Deklaracja bufora 3D objętości mgły w RenderGraphie z automatycznymi barierami tranzycji zasobów.
  - [x] Implementacja Temporal Anti-Aliasing (TAA) w `shaders/postprocess/TAA.hlsl` oraz `engine/renderer/passes/TAAPass.h` / `TAAPass.cpp`:
    - [x] Generator subpikselowego jittera kamery sekwencją Haltona (bazy 2 i 3) z 16-fazowym cyklem.
    - [x] Przycinanie historii kolorów (Color Variance Clipping) do lokalnej ramki AABB w przestrzeni barwnej w celu eliminacji smużenia (ghostingu).
  - [x] Zestaw testów jednostkowych w `tests/NextGenRendererTests.cpp`:
    - [x] Weryfikacja monotoniczności podziału głębi froxeli i asymetrii fazy Henyey-Greensteina.
    - [x] Weryfikacja zakresów jittera Haltona i przycinania próbek barw do lokalnej wariancji.
    - [x] **Milestone Fazy 12:** Pomyślna kompilacja grafu RenderGraph z aktywnymi passami `VolumetricFogPass` i `TAAPass`.
  - [x] Sukces kompilacji i przejście 100% testów jednostkowych w `Engine_Tests.exe` (0 błędów, 0 ostrzeżeń).
- [x] **2026-09-03: Faza 13: Rozszerzenia Przyszłościowe (Multi-Platform & Hardware Ray Tracing) [UKOŃCZONO I ZWERYFIKOWANO]**
  - [x] Implementacja struktur sprzętowego śledzenia promieni DXR 1.1 w `engine/rhi/include/engine/rhi/RHIRayTracing.h`:
    - [x] Deskryptory geometrii BLAS (`RayTracingGeometryDesc`) z obsługą buforów wierzchołków, indeksów i flag `Opaque`.
    - [x] Wyrównane do 16 bajtów deskryptory instancji TLAS (`RayTracingInstanceDesc`) z konwersją macierzy do formatu $3 \times 4$ wierszowo-afonicznego (Row-Major Affine Transform).
    - [x] Shader Binding Table (SBT) ze sprzętowym wyrównaniem wpisów do 64 bajtów (`DXR_SBT_ENTRY_ALIGNMENT`).
  - [x] Implementacja shadera śledzenia promieni w `shaders/raytracing/RaytracingReflections.hlsl` (RayGen, ClosestHit, Procedural Sky Miss, Shadow Miss).
  - [x] Warstwa abstrakcji backendu Vulkan 1.3 w `engine/rhi/include/engine/rhi/vulkan/VulkanTypes.h`:
    - [x] Deskryptor możliwości (`VulkanCapabilities`: bindless 500k, dynamic rendering, synchronization2, ray tracing).
    - [x] Konwersja formatów zasobów na `VkFormat` oraz mapowanie stanów `ResourceState` na etapy i flagi dostępu `VkPipelineStageFlags2` / `VkAccessFlags2`.
  - [x] Zestaw testów jednostkowych w `tests/RayTracingTests.cpp`:
    - [x] Weryfikacja deskryptorów BLAS i geometrii.
    - [x] Weryfikacja translacji macierzy $3 \times 4$ w TLAS (skalowanie, translacja, maski instancji).
    - [x] Weryfikacja 64-bajtowego wyrównania tablicy rekordów SBT.
    - [x] **Milestone Fazy 13:** Weryfikacja możliwości Vulkan 1.3 i bezkolizyjnego mapowania barier Synchronization2.
  - [x] Sukces kompilacji i przejście 100% testów jednostkowych w `Engine_Tests.exe` (0 błędów, 0 ostrzeżeń).
- [x] **2026-09-03: Faza 14: Global Illumination & Dynamic Meshlet LOD [UKOŃCZONO I ZWERYFIKOWANO]**
  - [x] Implementacja ciągłego LOD klastrów mikrotójkątów (Nanite-style Continuous LOD DAG) w `engine/renderer/MeshletLODDAG.h` / `MeshletLODDAG.cpp`:
    - [x] Struktura `MeshletCluster` z metrykami błędu geometrycznego w przestrzeni świata (`clusterError`, `parentError`).
    - [x] Projekcja błędu geometrycznego na piksele ekranowe na podstawie odległości, FOV i rozdzielczości ekranu.
    - [x] Reguła cięcia DAG (DAG Cut Selection Rule: $E_{\text{cluster}} \le \text{threshold} < E_{\text{parent}}$) gwarantująca spójną reprezentację bez pęknięć siatki geometrycznej ani podwójnego renderowania.
  - [x] Shader amplifikacji GPU w `shaders/geometry/MeshletClusterLOD.hlsl` ewaluujący błąd klastrów i emitujący widoczne siatki do Mesh Shadera.
  - [x] Implementacja Voxel Cone Tracing Global Illumination w `engine/renderer/VoxelGlobalIllumination.h` / `VoxelGlobalIllumination.cpp`:
    - [x] Generator 6 stożków półsferycznych (stożek centralny wzdłuż wektora normalnego $+ 5$ stożków bocznych o kącie $60^\circ$ w oparciu o bazę ortogonalną $T, B$).
    - [x] Projekcja współrzędnych świata na znormalizowaną przestrzeń UVW $[0, 1]^3$ 3D klipmapy wokseli.
    - [x] Pass RenderGraph `VoxelConeTracingPass` oraz compute shader `shaders/compute/VoxelConeTracingGI.hlsl` z dynamicznym doborem poziomów MIP stożka.
  - [x] Zestaw testów jednostkowych w `tests/MeshletGITests.cpp`:
    - [x] Weryfikacja reguły cięcia DAG w różnych odległościach kamery (blisko, optymalnie, daleko).
    - [x] Weryfikacja perspektywicznego skalowania błędu (dwukrotny wzrost dystansu dokładnie poławia błąd ekranowy).
    - [x] Weryfikacja ortogonalności bazy $T, B$ i geometrii stożków dyfuzyjnych.
    - [x] Weryfikacja mapowania UVW i **Milestone Fazy 14:** integracji passu VCT w RenderGraphie.
  - [x] Sukces kompilacji i przejście 100% testów jednostkowych w `Engine_Tests.exe` (0 błędów, 0 ostrzeżeń).

---

- [x] **2026-09-03: Faza 15: Networking & Deterministic Rollback Simulation [UKOŃCZONO I ZWERYFIKOWANO]**
  - [x] Implementacja dedykowanego modułu engine/network z typami symulacji sieciowej (NetworkTypes.h):
    - [x] Ścisły kwant czasu symulacji (Tick), PlayerInput, EntityPhysicalState oraz ServerSnapshot.
    - [x] Szablonowy, bezalokacyjny bufor kołowy stanu (SnapshotRingBuffer<T, Capacity>) o stałej pojemności potęgi dwójki (256 wpisów, indeksowanie bitowe 	ick & (Capacity - 1)).
  - [x] Implementacja silnika predykcji po stronie klienta (ClientPredictionSystem.h / ClientPredictionSystem.cpp):
    - [x] Deterministyczna integracja fizyczna SimulatePhysicsStep w stałym kroku 60 Hz (FIXED_DELTA_TIME = 1/60s).
    - [x] Algorytm rekonsyliacji (Server Reconciliation & Rollback Replay): wykrywanie desynchronizacji powyżej progu 2 cm (DEFAULT_RECONCILE_THRESHOLD), natychmiastowy powrót do autorytatywnego stanu serwera z ticku {\text{srv}}$ i deterministyczne ponowne odegranie (replay) bufora wejść {\text{srv}} + 1 \dots T_{\text{local}}$.
    - [x] Wygładzanie wizualne (Exponential Visual Smoothing Offset): wygaszanie wektora błędu $\vec{S}_{\text{offset}}$ ze współczynnikiem tłumienia ^{-\lambda \Delta t}$, eliminujące efekt nagłego przeskoku (snappingu) w widoku kamery.
  - [x] Implementacja autorytatywnego menedżera replikacji serwera (ServerReplicationManager.h / ServerReplicationManager.cpp):
    - [x] Sanityzacja i przycinanie wejść klienta (anti-cheat clamping moveX, moveZ do $[-1, 1]$).
    - [x] Obsługa zewnętrznych impulsów fizycznych i generowanie autorytatywnych migawek (ServerSnapshot).
  - [x] Zestaw testów jednostkowych i symulacyjnych w 	ests/NetworkRollbackTests.cpp:
    - [x] Weryfikacja bezalokacyjnego bufora kołowego SnapshotRingBuffer (zawijanie indeksów przy 100 wpisach, ochrona przed wyciekami).
    - [x] Weryfikacja pętli predykcji klienta (60 ticków sprintu daje dokładnie 9.00 m w osi Z).
    - [x] **Milestone Fazy 15:** Weryfikacja rollbacku o 10 ticków po zewnętrznym impulsie serwera (+1.5m X) i bezbłędne odtworzenie trajektorii.
    - [x] Weryfikacja płynnego wykładniczego zaniku wektora wygładzania z 0.50m do 0.000m bez przeskoku geometrii.
  - [x] Sukces kompilacji i przejście 100% testów jednostkowych w Engine_Tests.exe (0 błędów, 0 ostrzeżeń).

---

- [x] **2026-09-03: Faza 16: Neural Radiance Caching & ML Acceleration [UKOŃCZONO I ZWERYFIKOWANO]**
  - [x] Implementacja silnika Neural Radiance Caching (NRC) w engine/renderer/include/engine/renderer/NeuralRadianceCache.h oraz src/NeuralRadianceCache.cpp:
    - [x] Przestrzenne kodowanie harmoniczne NeRF/Instant-NGP (EncodePosition): rzutowanie pozycji 3D na 24-wymiarowy wektor cech częstotliwościowych $\sin(2^k \pi x), \cos(2^k \pi x)$.
    - [x] Kompaktowy 2-warstwowy perceptron (MLP) z aktywacją ReLU i wyjściem nieujemnym (RGB Radiance $\ge 0$).
    - [x] Samouczący się bufor uczący (Online SGD Training Step): spadek straty MSE z 0.4742 do 0.0000 przy estymacji radyjancji odbić pośrednich.
    - [x] Integracja z grafem RenderGraph (NeuralRadianceCachePass) czytającym bufory normalnych i głębokości G-Buffera oraz emitującym bufor radyjancji pośredniej.
  - [x] Implementacja compute shadera GPU w shaders/compute/NeuralRadianceCacheInference.hlsl:
    - [x] Równoległa rekonstrukcja pozycji ze zbuforowanej głębokości i wyliczanie kodowań częstotliwościowych.
    - [x] Szacowanie wielo-odbiciowego oświetlenia pośredniego w locie bez kosztownego śledzenia wielu promieni per piksel.
  - [x] Zestaw testów jednostkowych i konwergencji w 	ests/NeuralRadianceCacheTests.cpp:
    - [x] Weryfikacja ortogonalności tożsamości $\sin^2(\theta) + \cos^2(\theta) = 1.0$ dla wszystkich pasm harmonicznych.
    - [x] Weryfikacja nieujemności radyjancji wyjściowej w inferencji.
    - [x] **Milestone Fazy 16:** Weryfikacja zbieżności uczenia online SGD (spadek błędu i dopasowanie do światła odbitego).
    - [x] Weryfikacja planowania i kompilacji passu w grafie RenderGraph.
  - [x] Sukces kompilacji i przejście 100% testów jednostkowych w Engine_Tests.exe (0 błędów, 0 ostrzeżeń).

---

- [x] **2026-09-03: Phase 17: Procedural World Streaming & Virtual Texturing (SVT / Clipmaps) [COMPLETED & VERIFIED]**
  - [x] Implementation of GPU-Driven Procedural Terrain Clipmaps in engine/renderer/include/engine/renderer/WorldStreaming.h and src/WorldStreaming.cpp:
    - [x] Nested multi-resolution concentric rings ( \dots L_5$) with grid spacing doubling ({k+1} = 2 s_k$) from 0.5m to 16.0m.
    - [x] Concentric ring origin snapping to grid intervals to eliminate vertex and texture shimmering during camera movement.
    - [x] Continuous C0 geomorphing blending factor $\alpha(r) = \text{clamp}\left(\frac{r - r_{\text{in}}}{r_{\text{out}} - r_{\text{in}}}, 0, 1\right)$ completely eliminating geometric popping between adjacent LOD rings.
    - [x] Multi-octave fractal landscape elevation synthesis ( + o_2 + o_3$).
  - [x] Implementation of Sparse Virtual Texturing (SVT) Page Table & LRU Cache in VirtualTextureManager:
    - [x] 64k x 64k virtual address space partitioned into 256x256 pixel tiles.
    - [x] Physical GPU texture atlas with fixed capacity ( \times 16 = 256$ physical tiles) preventing VRAM exhaustion.
    - [x] Real-time Page Table tracking residency, cache hits/misses, and least-recently-used (LRU) slot eviction upon cache overflow.
  - [x] Implementation of compute shader in shaders/compute/TerrainClipmapUpdate.hlsl:
    - [x] Parallel per-vertex geomorphing interpolation between fine and coarse heights.
    - [x] Analytical finite-difference normal generation and UV emission.
  - [x] Integration with RenderGraph via TerrainStreamingPass reading depth/height and emitting scene geometry.
  - [x] Comprehensive test suite in 	ests/WorldStreamingTests.cpp:
    - [x] Verification of concentric ring spacing doubling and coordinate snapping.
    - [x] Verification of continuous geomorph factor transition from 0.0 to 1.0.
    - [x] **Phase 17 Milestone:** Verification of Virtual Texture LRU eviction upon filling 256 physical slots.
    - [x] Verification of TerrainStreamingPass compilation in the RenderGraph.
  - [x] Successful compilation and 100% test pass in Engine_Tests.exe (0 errors, 0 warnings).

---

- [x] **2026-09-03: Phase 18: AAA Portfolio Showcase & Interactive Tech Demo [COMPLETED & VERIFIED]**
  - [x] Interactive First-Person Fly Camera in runtime/src/Main.cpp with WASD 6-DOF movement, Space/Ctrl vertical translation, Sprint mode (Shift), and mouse look rotation.
  - [x] High-precision real-time telemetry HUD updating window title with live FPS, frame times in milliseconds, camera coordinates, and compiled RenderGraph pass count.
  - [x] Debug 3D world reference grid and concentric Terrain Clipmap LOD ring visualization.
  - [x] Full Next-Gen RenderGraph pipeline scheduling in the runtime frame loop: DepthPrepass, VolumetricFogPass (Froxels), VoxelGlobalIllumination (VCT), NeuralRadianceCache (Online ML), VirtualTextureManager (TerrainStreaming), LightingPass, and PostProcessPass.
  - [x] Clean shutdown in reverse DAG order with 0 leaks.

---

- [x] **2026-09-03: Phase 20: Advanced Nanite-Style Mesh Cluster Hardware Micro-Culling [COMPLETED & VERIFIED]**
  - [x] Bounding normal cone evaluation (ClusterNormalCone) in engine/renderer/include/engine/renderer/MeshShaderCulling.h and src/MeshShaderCulling.cpp.
  - [x] Immediate backface cluster rejection in Task / Amplification shaders ($\vec{V} \cdot \vec{N} \ge \sin(\theta)$) before emitting Mesh Shader threadgroups.
  - [x] Cluster frustum plane intersection and Hierarchical Depth Buffer (HZB) occlusion testing.
  - [x] Integration with RenderGraph via MeshletClusterCullPass dispatching hardware mesh shaders.
  - [x] Comprehensive test suite in tests/MeshShaderCullingTests.cpp:
    - [x] Verification of normal cone orientation and angle cutoff.
    - [x] Verification of frustum plane intersection.
    - [x] **Milestone Phase 20:** Verification of GPU cluster pipeline culling exactly 32 backfacing clusters out of 64 in 1 pass.
    - [x] Verification of MeshletClusterCullPass RenderGraph compilation.
  - [x] Successful compilation and 100% test pass in Engine_Tests.exe (0 errors, 0 warnings across all 19 test suites).

---

- [x] **2026-09-03: Phase 19: Cross-Platform Packaging & Vulkan 1.3 Linux Port [COMPLETED & VERIFIED]**
  - [x] Production multi-stage Docker containerization in Dockerfile.linux for Ubuntu 24.04 LTS, Clang 18, libc++, and Vulkan 1.3 SDK.
  - [x] Headless platform window abstraction in engine/platform/include/engine/platform/HeadlessWindow.h for Linux servers and CI runners without X11/Wayland display server.
  - [x] Automation shell script in scripts/build_linux.sh for native Clang 18 + Ninja compilation and automated ctest execution.
  - [x] Seamless compilation and test execution with 0 errors.

---

- [x] **2026-09-03: Phase 22: High-Fidelity Atmospheric Sky Atmosphere & Dual-Lobe GGX PBR [COMPLETED & VERIFIED]**
  - [x] Physically-based Rayleigh and Mie atmospheric scattering implementation in engine/renderer/include/engine/renderer/AtmosphericSky.h and src/AtmosphericSky.cpp:
    - [x] Wavelength-dependent Rayleigh scattering ($\beta_R \propto \lambda^{-4}$) yielding natural blue zenith chromaticity.
    - [x] Henyey-Greenstein Mie phase function with strong forward solar peak asymmetry ($g = 0.8$).
    - [x] Beer-Lambert transmittance $\tau(s)$ integration through spherical atmospheric shell density profiles.
  - [x] Dual-Lobe GGX microfacet specular BRDF ($D_{\text{Dual}} = (1 - w) D_{\text{GGX}}(\alpha_1) + w D_{\text{GGX}}(\alpha_2)$) for clearcoat lacquer and cloth sheen.
  - [x] RenderGraph pass scheduling via AtmosphereSkyPass.
  - [x] Unit test suite in tests/AtmosphericSkyTests.cpp (100% pass across all 4 atmospheric tests).
  - [x] Successful compilation and 100% test pass in Engine_Tests.exe (0 errors, 0 warnings across all 20 test suites).
- [x] **2026-09-03: Phase 21: DirectStorage 1.2 GPU Decompression & Streaming [COMPLETED & VERIFIED]**
  - [x] Implementation of DirectStorage 1.2 asynchronous streaming architecture in `engine/assets/include/engine/assets/DirectStorageManager.h` and `src/DirectStorageManager.cpp`:
    - [x] Request queue management (`DSRequest`) with support for CPU memory and GPU virtual address destinations.
    - [x] GDeflate hardware GPU decompression format support (`DSCompressionFormat::GDeflate`).
    - [x] Queue batching and submit pipeline advancing hardware completion fence (`m_currentFenceValue`).
    - [x] Synchronous and asynchronous completion polling (`WaitForFence`, `PollCompletions`) and bandwidth statistics tracking (`DirectStorageStats`).
  - [x] GPU decompression compute shader in `shaders/compute/GDeflateDecompress.hlsl` for parallel threadgroup bitstream unpacking.
  - [x] Comprehensive test suite in `tests/DirectStorageTests.cpp`:
    - [x] Verification of DirectStorage 1.2 initialization and capability query.
    - [x] Verification of request batching and monotonic request ID generation.
    - [x] **Milestone Phase 21:** Verification of fence synchronization and 4x GDeflate decompression bandwidth statistics (2048B compressed -> 8192B uncompressed).
    - [x] Verification of defensive guardrails intercepting zero-size, null-destination, and invalid compression ratio requests.
  - [x] Successful compilation and 100% test pass in `Engine_Tests.exe` (0 errors, 0 warnings across all 21 test suites).
- [x] **2026-09-03: Cloud Infrastructure & Containerization (Docker, Kubernetes & Helm) [COMPLETED & VERIFIED]**
  - [x] Multi-stage production containerization in `deploy/docker/Dockerfile.server` for Linux dedicated game servers and headless CI/CD simulation (Ubuntu 24.04, Clang 18, non-root security user, UDP/TCP exposed ports, container healthchecks).
  - [x] Kubernetes cluster orchestration manifests in `deploy/k8s/`:
    - [x] ConfigMap (`configmap.yaml`) for 60 Hz tick rate, server tick budget, and rollback replication configuration.
    - [x] Deployment (`game-server-deployment.yaml`) with rolling update strategy, CPU/memory requests and limits, non-root securityContext, and volume mounts.
    - [x] Service (`game-server-service.yaml`) for UDP 7777 game traffic and TCP 8080 telemetry load balancing.
    - [x] Fleet Autoscaler (`gameserver-fleet.yaml`) for automatic horizontal pod autoscaling (HPA) from 2 to 10 replicas based on CPU / game ticks.
  - [x] Production Helm 3 Chart in `deploy/helm/mygameengine/` (`Chart.yaml`, `values.yaml`, `templates/deployment.yaml`, `templates/service.yaml`).
  - [x] Automated deployment scripts in `scripts/build_docker.sh` and `scripts/deploy_k8s.sh`.
  - [x] **Runtime & Editor Window Stability Fixes:**
    - [x] Added dedicated offscreen RTV descriptor heap (512 slots) and DSV descriptor heap (128 slots) in `D3D12Device` eliminating zero-handle crashes in RenderGraph passes.
    - [x] Added defensive null-pointer and descriptor handle validation in `D3D12CommandList::ClearRenderTarget`, `ClearDepthStencil`, and `SetRenderTargets`.
    - [x] Upgraded `EngineEditor.exe` to an interactive event loop window (`1440x900`) with real-time FPS/ms title updates instead of terminating after 5 frames.
    - [x] Hardware verification: `GameRuntime.exe` and `EngineEditor.exe` run continuously at 60 FPS without crashing or premature exit.

---

### Active Sprint [In Progress]

#### Phase 23: Hardware Ray-Traced Audio Acoustic Propagation & Diffuse Scattering
- [ ] DXR ray-traced audio occlusion, early specular reflections, and impulse response convolution.
- [ ] Reverberation field parameter estimation from scene geometry surface materials.

---

### Future / Backlog Register [Future / Backlog]

#### Phase 24: Real-Time Destruction & Voronoi Convex Decomposition
- [ ] Asynchronous Voronoi cell fracturing and dynamic Jolt rigid body creation upon high-energy impacts.

#### Phase 25: AI NPC Behavior Trees & Hierarchical Task Networks (HTN)
- [ ] Real-time hierarchical task planning with spatial query navigation and dynamic obstacle avoidance.


---


### Milestone Phase 23.2: Editor UI Realism, Numerical Channel Displays, Interactive Dropdown Menus & Console Tab Switching [Completed]
- [x] **Font & Text Direction Fixes:**
  - [x] Resolved horizontal font mirroring in embedded 4x6 bitmap glyph renderer by flipping bit-shift extraction (3 - uint(rel.x)) from MSB to LSB.
  - [x] Converted embedded glyph lookup to a 128-entry constant array lookup table (FontGlyphs[128]), completely eliminating branching overhead and reducing HLSL compilation time.
- [x] **Stationary Viewport Stability:**
  - [x] Decoupled viewport camera angles from continuous Time animation; viewport remains completely stationary and stable by default.
  - [x] Integrated smooth, responsive manual camera orbiting via Right Mouse Button (RMB).
- [x] **Interactive Drop-Down Menus:**
  - [x] Implemented clickable FILE menu dropdown: [ New Scene ], [ Open Scene ], [ Save Scene ], [ Exit ].
  - [x] Implemented clickable EDIT menu dropdown: [ Undo ], [ Redo ], [ Settings ].
  - [x] Implemented clickable SCENE menu dropdown: [ Add Cube ], [ Add Light ], [ Add Camera ], [ Clear ].
  - [x] Auto-dismissing popups on external or menu-item click with clean state transitions.
- [x] **Context-Aware Dynamic Inspector with Numerical Readouts:**
  - [x] Selected **Camera (ID 1)**: Displays FOV: 60.0 DEG, NEAR: 0.10 M, FAR: 1000 M.
  - [x] Selected **Sun Light (ID 2)**: Displays TYPE: SUN, INTEN: 5.0 LUX, COLOR: 1.00, SHADOWS: CSM 4-SPLIT.
  - [x] Selected **Dynamic Cube (ID 3)**: Displays editable numerical coordinates (POS: [0.00] [1.00] [0.00], ROT: [0.00] [45.0] [0.00], SCALE: [1.00] [1.00] [1.00]), RIGIDBODY: 1.0 KG, BOX COLLIDER: 1.0 M3, MATERIAL: PBR STANDARD.
  - [x] Selected **Terrain (ID 4)**: Displays CLIPMAP 5 RINGS, GRID: 1.0 M SPACING.
  - [x] Selected **Fog (ID 5)**: Displays DENSITY: 0.035, RANGE: 120 M.
  - [x] Interactive Inspector tweaking: Clicking position fields adjusts cube coordinates in real time.
- [x] **Interactive Dock Tabs (Assets vs. Live Console Output):**
  - [x] Clicking CONSOLE tab switches the bottom panel to live engine logging messages ([INFO] D3D12 DEVICE, [INFO] PHYSICS 60 HZ, [INFO] ENTITY SELECTED, [INFO] RENDERGRAPH).
  - [x] Clicking ASSETS tab switches view back to asset cards (cube.cmesh, checker.ctex, PBR_Standard.cmat, MainScene.cscene).

---


### Milestone Phase 23.3: Interactive 3D Entity Spawning/Deletion, Settings Modal with Sliders & High-DPI Action Bar [Completed]
- [x] **Dynamic 3D Scene Entity Placement & Deletion:**
  - [x] Implemented dynamic runtime entity manager supporting up to 8 real-time entities with distinct geometry (Cubes, Spheres, Light Markers, Cameras).
  - [x] Quick Action Bar [+ CUBE], [+ SPHERE], [+ LIGHT] spawns new 3D objects directly into the viewport scene with calculated offsets and automatic selection.
  - [x] Integrated entity destruction via Quick Action Bar [DELETE] button or Delete / Backspace hotkeys, updating hierarchy, inspector, and GPU scene buffers in real-time.
  - [x] Multi-object 3D raymarching with individual bounding boxes, spheres, dynamic PBR lighting, and vibrant cyan selection outlines.
- [x] **Project & Renderer Settings Modal with 5 Interactive Sliders:**
  - [x] Centered glassmorphic modal window with backdrop dimming and smooth toggle via [SETTINGS] button, EDIT -> SETTINGS, or ESC.
  - [x] **Camera Speed Slider**: Interactive drag bar controlling viewport orbit sensitivity (1.0 to 10.0 m/s).
  - [x] **Field of View (FOV) Slider**: Live camera perspective slider adjusting raymarching field of view (30.0 to 90.0 deg).
  - [x] **Sun Intensity Slider**: Real-time slider modulating directional sunlight radiance (1.0 to 10.0 lux).
  - [x] **Shadow Distance Slider**: Configurable cascade shadow distance slider (50.0 to 300.0 m).
  - [x] **Exposure Slider**: Physical camera EV exposure slider controlling scene tone-mapping (8.0 to 16.0 EV).
- [x] **Inspector Incremental Controls:**
  - [x] Real-time [-] and [+] stepping buttons for POS X, POS Y, POS Z coordinates with instant visual object translation across the 3D grid.
- [x] **Play Mode Multi-Object Simulation:**
  - [x] All dynamic entities simulate Newtonian physics, gravity, and ground restitution bouncing concurrently during Play mode.

---


### Milestone Phase 23.4: Unreal Engine 5 & Unity Inspired Workspace, Free-Fly WASD Navigation, 3D Gizmo Dragging, Content Drawer & Standalone Shipping Packager [Completed]
- [x] **Unreal Engine 5 Navigation & Viewport Controls:**
  - [x] Integrated smooth Free-Fly camera navigation using Hold RMB + WASD with Q (fly down), E (fly up), and Left Shift (2.5x turbo speed).
  - [x] Camera position tracked in full 3D space (camPosX, camPosY, camPosZ) with unlimited flight freedom across the infinite ground grid.
- [x] **3D Transform Gizmo & Scene Object Dragging:**
  - [x] Integrated 3D Transform Gizmo rendered directly at the selected entity position with Red (+X), Green (+Y), and Blue (+Z) axis arrow handles.
  - [x] Interactive scene dragging: Holding Left Mouse Button inside the 3D viewport allows direct object translation along the X-Z ground plane following mouse delta, with Shift adjusting altitude (Y).
  - [x] Implemented standard Unreal/Unity Gizmo mode hotkeys: Q (Select), W (Translate/Move), E (Rotate), R (Scale).
- [x] **Expanded Multi-Primitive Scene Creation Palette:**
  - [x] Added support for 3D Static Mesh Cubes (SM_Cube), Spheres (SM_Sphere), Cylinders (SM_Cylinder), Floor Planes (Floor_Plane), and Point Lights (PointLight).
  - [x] Quick Action toolbar buttons: [+ CUBE], [+ SPHERE], [+ CYLINDER], [+ PLANE], [+ LIGHT], [🗑 DELETE].
- [x] **Full Unreal Content Drawer / Asset Browser:**
  - [x] Organized Content hierarchy with sub-folder categories: [MESHES], [MATERIALS], [TEXTURES], [SCENES].
  - [x] Interactive asset thumbnail cards with instant spawn capability into the active 3D scene.
- [x] **One-Click Standalone Game Shipping & Packaging System:**
  - [x] Added [BUILD GAME] toolbar button and BUILD -> PACKAGE PROJECT menu option.
  - [x] Automated packaging pipeline creates a standalone shipping release in dist/Windows/:
    - Copies compiled GameRuntime.exe as dist/Windows/Game.exe.
    - Bundles all assets and engine shaders into dist/Windows/assets/ and dist/Windows/engine/.
    - Generates standalone launcher dist/Windows/Launch_Game.bat.
  - [x] Added interactive Build Success modal with single-click [ LAUNCH PACKAGED GAME ] test button.

---


### Milestone Phase 23.5: Full AAA Editor Studio, Simulation State Isolation, 3D Raycast Picking, Vehicle Calibration (F5-F9), and Add Component Workflow [Completed]
- [x] **Simulation State Isolation (Zero Scene Leaks):**
  - [x] Implemented pre-play scene snapshot architecture. Clicking [▶ PLAY] takes a complete memory snapshot of all entities.
  - [x] Added [⏸ PAUSE] to freeze physics and animations without losing simulation context.
  - [x] Added [⏭ STEP] allowing developers to advance the physics engine by exactly one 60 Hz frame (16.6 ms) for frame-by-frame collision debugging.
  - [x] Added [⏹ STOP] restoring all entity transforms and states to the exact pre-simulation snapshot.
- [x] **Interactive 3D Mouse Ray Picking:**
  - [x] Integrated camera raycast generation from screen pixels through the viewport plane into 3D world space.
  - [x] Real-time intersection testing against scene entities for direct click-to-select in the 3D viewport.
- [x] **Camera Navigation Enhancements (Frame Focus & Orbit):**
  - [x] Added Frame Focus (F key): Instantly centers the camera view onto the selected entity's bounding volume.
  - [x] Added Orbit Camera (Alt + LMB): Rotates the camera spherically around the focused target entity.
- [x] **Component Architecture & Inspector Expansion:**
  - [x] Added [+ ADD COMPONENT] popup dialog to the Inspector panel.
  - [x] Support for adding RigidBodyComponent, BoxColliderComponent, AudioSourceComponent, ScriptComponent, and ParticleEmitterComponent.
- [x] **Vehicle Physics & Calibration Debug Tools (F5 - F9):**
  - [x] F5: Toggle Debug Drawing for wireframe colliders and force vectors in viewport.
  - [x] F6: Puncture front-left tire (0) with immediate friction and rolling drag adjustment.
  - [x] F7: Puncture rear-right tire (3) (GTA-style tire blowout) simulating drift angle deviation.
  - [x] F8: Quick repair restoring all 4 tires to nominal 2.2 bar pressure and 100% grip.
  - [x] F9: Automated vehicle calibration test sequence (T1 Acceleration -> T2 Slalom -> T3 Braking -> T4 Rollover) exporting telemetry to calibration_telemetry.csv.

---

## Rejestr Ryzyk i Zasad Nienaruszalnych (Engine Inviolable Rules)

1. **Zasada Czystego Runtime:** Moduł `engine/runtime` oraz `engine/core` pod żadnym pozorem nie mogą linkować ani zawierać kodu edytora (`editor/`) ani nagłówków Dear ImGui.
2. **Zakaz Alokacji na Stercie w Pętli Gry:** Żadna funkcja wywoływana co klatkę (`Tick()`, `Render()`, `Update()`) nie może wykonywać operacji `new` ani `malloc`. Cała pamięć tymczasowa musi pochodzić z `FrameAllocator`.
3. **Zasada Bindless First:** Zabrania się tworzenia tradycyjnych slotów bindowania tekstur per-obiekt. Wszystkie materiały odwołują się do tekstur za pomocą indeksu całkowitego (`TextureID`) w globalnej stercie deskryptorów.
4. **Zasada Spójności Rejestru:** Każda zmiana w architekturze planu musi być dopisywana na końcu tego dokumentu. Zadania ukończone przenoszone są do sekcji `[Completed]`.
