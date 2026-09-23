# Modern 3D Game Engine (Standard 2026) — Agile Master Product Backlog & Roadmap

> **Methodology Framework:** Agile Scrum / Kanban  
> **Target Standard:** AAA Senior Engine & Graphics Programmer (C++20/C++23, DirectX 12 Agility SDK, Vulkan 1.3)  
> **Key Architectural Themes:** Zero-Allocation Render Loop, GPU-Driven Pipelines (100k instances), FrameGraph, Hardware Ray Tracing (DXR), Meshlet Continuous LOD DAG, Voxel Cone Tracing GI, Deterministic Network Rollback & Reconciliation.

---

## 🎯 Wizja Produktu (Product Vision) & Uzasadnienie Biznesowo-Techniczne ("PO CO?")

Celem projektu jest stworzenie bezkompromisowo zoptymalizowanego silnika 3D nowej generacji, który eliminuje fundamentalne wąskie gardła współczesnych silników komercyjnych (Unity/Unreal Engine):
1. **Zero dynamicznych alokacji pamięci w pętli gry** (Custom Memory Arenas: `LinearAllocator`, `FrameAllocator`, `PoolAllocator`).
2. **GPU-Driven Pipeline z cullingiem 100 000 instancji w czasie < 1.5 ms** (ExecuteIndirect, HZB Occlusion, Work-Graphs).
3. **Wielowątkowy Job-Centric Task Graph z Work-Stealing** skalujący się liniowo na wielordzeniowe procesory.
4. **Wielowątkowa fizyka Jolt 5.0** zsynchronizowana z harmonogramem zadań silnika.
5. **Deterministyczny silnik sieciowy (Deterministic Network Rollback & Client-Side Prediction)** w stałym kroku 60 Hz z bezalokacyjnym buforem kołowym migawek (`SnapshotRingBuffer`).

---

## 📊 Tablica Agile (Sprint Board Overview)

```
┌───────────────────────────────────────┬───────────────────────────────────────┬───────────────────────────────────────┐
│        BACKLOG / CO DALEJ             │        W TRAKCIE (IN PROGRESS)        │          ZROBIONE (COMPLETED)         │
├───────────────────────────────────────┼───────────────────────────────────────┼───────────────────────────────────────┤
│ • Epik 23: Ray-Traced Audio Acoustic  │ • Epik 21: DirectStorage 1.2 GPU Decom│ • Epik 0: Core, Alokatory, Job System │
│ • Epik 24: Voronoi Real-Time Fracturin│   (GDeflate & Zero-Copy Streaming)    │ • Epik 1: RHI DX12 & SwapChain        │
│                                       │                                       │ • Epik 2: DOD ECS (Sparse-Set 50k)    │
│                                       │                                       │ • Epik 3: Asset Pipeline & Async VFS  │
│                                       │                                       │ • Epik 4: Render Graph (FrameGraph)   │
│                                       │                                       │ • Epik 5: Edytor Dokowalny ImGui      │
│                                       │                                       │ • Epik 6: Wielowątkowa Fizyka Jolt    │
│                                       │                                       │ • Epik 7: GPU-Driven Pipeline 100k    │
│                                       │                                       │ • Epik 8: Animacja & Audio 3D         │
│                                       │                                       │ • Epik 9: RenderDoc & Telemetria      │
│                                       │                                       │ • Epik 10: Optymalizacje LTO & ASan   │
│                                       │                                       │ • Epik 12: Next-Gen Fog & TAA         │
│                                       │                                       │ • Epik 13: Sprzętowy DXR & Vulkan 1.3 │
│                                       │                                       │ • Epik 14: Meshlet LOD & Voxel GI     │
│                                       │                                       │ • Epik 15: Network Rollback 60 Hz     │
│                                       │                                       │ • Epik 16: Neural Radiance Cache      │
│                                       │                                       │ • Epik 17: Procedural World Streaming │
│                                       │                                       │ • Epik 18: AAA Tech Demo Showcase     │
│                                       │                                       │ • Epik 19: Linux Vulkan 1.3 Headless  │
│                                       │                                       │ • Epik 20: Mesh Shader Micro-Culling  │
│                                       │                                       │ • Epik 22: Atmospheric Sky & Dual-Lobe│
└───────────────────────────────────────┴───────────────────────────────────────┴───────────────────────────────────────┘
```

---

## ✅ Zrealizowane Epiki Produktowe (Completed Sprints)

### Epik 0: Fundament Silnika, System Pamięci i Task Graph
- **User Story 0.1:** Jako programista silnika, chcę używać niestandardowych alokatorów pamięci (`LinearAllocator`, `FrameAllocator`, `PoolAllocator`), aby wyeliminować kosztowne wywołania systemowego `malloc`/`new` i fragmentację pamięci RAM w pętli gry.
  - **PO CO (Value):** Gwarancja stałego czasu klatki 16.6 ms (60 FPS) bez mikroprzycięć związanych z alokatorem OS.
- **User Story 0.2:** Jako programista systemowy, chcę wielowątkowego Job Systemu z kradzieżą zadań (Work-Stealing Deque) i `ParallelFor`, aby skalować obliczenia symulacji na wszystkie rdzenie procesora.
  - **PO CO (Value):** 100% utylizacji nowoczesnych wielordzeniowych procesorów bez blokowania wątku głównego.
- **Status:** `[x] ZROBIONE` (`tests/AllocatorTests.cpp`, `tests/JobSystemTests.cpp` — 100% Passed).

### Epik 1: Niskopoziomowy RHI (DirectX 12 Agility SDK)
- **User Story 1.1:** Jako inżynier renderingu, chcę nowoczesnej warstwy abstrakcji RHI opartej o Bindless Resource Indexing (500 000 slotów deskryptorów), aby nie przełączać slotów deskryptorów per-obiekt na CPU.
  - **PO CO (Value):** Redukcja CPU Draw Call Overhead o rząd wielkości.
- **Status:** `[x] ZROBIONE` (`engine/rhi`, `tests/RHITests.cpp` — fizyczne renderowanie 5 klatek na GPU NVIDIA GTX 1660 SUPER).

### Epik 2: Architektura Danych Data-Oriented Design & Sparse-Set ECS
- **User Story 2.1:** Jako projektant rozgrywki, chcę przechowywać komponenty encji w ciągłych buforach pamięci (Sparse-Set), aby iteracje po transformacjach i fizyce były przyjazne dla pamięci podręcznej CPU (L1/L2/L3 Cache-Friendly).
  - **PO CO (Value):** Zero cache-misses przy aktualizacji 50 000 encji (wynik: 10.33 ms na 1 rdzeniu = odpowiednik 97 FPS).
- **Status:** `[x] ZROBIONE` (`engine/world/SparseSet.h`, `tests/WorldTests.cpp`).

### Epik 3: Asynchroniczny Asset Pipeline & VFS
- **User Story 3.1:** Jako silnik, chcę kompilować modele i tekstury do natywnych formatów binarnych (`AssetCooker`) oraz wczytywać je asynchronicznie przez ring-buffer I/O, aby nie blokować klatki renderowania podczas doczytywania świata.
  - **PO CO (Value):** Płynna rozgrywka bez zacięć (zero frame hitching).
- **Status:** `[x] ZROBIONE` (`tests/AssetTests.cpp`).

### Epik 4: Render Graph (FrameGraph) & PBR Pipeline
- **User Story 4.1:** Jako grafik, chcę aby przejścia renderera były deklarowane w postaci acyklicznego grafu (Render Graph), który automatycznie wylicza bariery zasobów (`D3D12_RESOURCE_BARRIER`), wykrywa cykle i eliminuje martwe przejścia (dead-code culling).
  - **PO CO (Value):** Eliminacja hazardów GPU, wygoda rozbudowy pipeline'u i oszczędność VRAM.
- **Status:** `[x] ZROBIONE` (`engine/renderer/RenderGraph.cpp`, `tests/RenderGraphTests.cpp`).

### Epik 5: Zintegrowany Edytor Dokowalny (Dear ImGui)
- **User Story 5.1:** Jako projektant poziomów, chcę wielookienkowy edytor z panelami Scene Hierarchy, Inspector, Viewport z gizmo 3D i Profilerem, aby debugować scenę w czasie rzeczywistym.
  - **PO CO (Value):** Błyskawiczny feedback loop i komfort pracy nad grą.
- **Status:** `[x] ZROBIONE` (`editor/src/EditorApp.cpp`, `tests/EditorTests.cpp`).

### Epik 6: Wielowątkowa Fizyka Jolt 5.0
- **User Story 6.1:** Jako programista fizyki, chcę zintegrować bibliotekę Jolt Physics z naszym `JobSystemem`, aby kolizje tysięcy ciał sztywnych były liczone równolegle w wątkach roboczych.
  - **PO CO (Value):** Stabilna symulacja fizyczna w 60 Hz bez spowalniania renderera.
- **Status:** `[x] ZROBIONE` (`engine/physics`, `tests/PhysicsTests.cpp`).

### Epik 7: GPU-Driven Pipeline & Test 100 000 Instancji
- **User Story 7.1:** Jako inżynier grafiki, chcę realizować culling brył widzenia (Frustum Culling), wielopoziomowe LOD oraz hierarchiczne odrzucanie zasłoniętych obiektów (Hierarchical Z-Buffer / HZB) w shaderach compute na GPU, wykonując pojedyncze wywołanie `ExecuteIndirect`.
  - **PO CO (Value):** 100 000 obiektów przefiltrowanych i odrzuconych na GPU w czasie **1.14 ms** bez angażowania CPU.
- **Status:** `[x] ZROBIONE` (`tests/GPUDrivenTests.cpp`).

### Epik 8: Animacja Szkieletowa & Dźwięk Przestrzenny
- **User Story 8.1:** Jako inżynier animacji, chcę odkształcać siatki postaci (Mesh Skinning) w compute shaderze GPU i próbkować klipy animacji, aby renderować armie postaci bez narzutu na procesor.
- **User Story 8.2:** Jako gracz, chcę słyszeć dźwięki pozycyjne w przestrzeni 3D z uwzględnieniem odległości i orientacji słuchacza.
- **Status:** `[x] ZROBIONE` (`engine/animation`, `engine/audio`, `tests/AnimationAudioTests.cpp`).

### Epik 9: Telemetria, Profiler i Narzędzia Diagnostyczne
- **User Story 9.1:** Jako inżynier wydajności, chcę integracji z RenderDocem w locie (`F12`) oraz panelu metryk VRAM i czasów CPU/GPU.
- **Status:** `[x] ZROBIONE` (`engine/core/Profiler.h`, `tests/ProfilerTests.cpp`).

### Epik 10: Optymalizacje Produkcyjne i Higiena Kodu
- **User Story 10.1:** Jako architekt, chcę obsługi flag LTO/IPO w CMake, czystego cyklu życia DAG komponentów runtime oraz zero wycieków pamięci.
- **Status:** `[x] ZROBIONE` (`tests/DistributionTests.cpp`).

### Epik 12: Next-Gen Renderer (Volumetric Fog & TAA)
- **User Story 12.1:** Jako inżynier grafiki, chcę mgły wolumetrycznej opartej na siatce froxeli z asymetrią Henyey-Greensteina oraz wygładzania TAA z jitterem Haltona i przycinaniem wariancji kolorów (Variance Clipping).
- **Status:** `[x] ZROBIONE` (`shaders/compute/VolumetricFog.hlsl`, `shaders/postprocess/TAA.hlsl`, `tests/NextGenRendererTests.cpp`).

### Epik 13: Sprzętowe Śledzenie Promieni (DXR 1.1) & Vulkan 1.3
- **User Story 13.1:** Jako programista RHI, chcę deskryptorów geometrii BLAS, instancji TLAS z 16-bajtowym wyrównaniem, tabeli Shader Binding Table (64-bajtowe wyrównanie) oraz mapowania barier Synchronization2 dla Vulkana 1.3.
- **Status:** `[x] ZROBIONE` (`engine/rhi/RHIRayTracing.h`, `VulkanTypes.h`, `tests/RayTracingTests.cpp`).

### Epik 14: Global Illumination & Dynamic Meshlet LOD
- **User Story 14.1:** Jako inżynier grafiki, chcę ciągłego LOD klastrów mikrótrójkątów (Nanite-style Continuous LOD DAG) z regułą cięcia drzewa DAG oraz Voxel Cone Tracing GI z 6 stożkami próbkującymi klipmapę 3D.
- **Status:** `[x] ZROBIONE` (`engine/renderer/MeshletLODDAG.h`, `VoxelGlobalIllumination.h`, `tests/MeshletGITests.cpp`).

### Epik 15: Deterministyczny Silnik Sieciowy (Deterministic Rollback & 60 Hz Reconciliation)
- **User Story 15.1:** Jako gracz sieciowy, chcę aby silnik natychmiast reagował na moje sterowanie (Client-Side Prediction) przy użyciu stałego kroku symulacji 60 Hz, aby wyeliminować opóźnienie percepcyjne w grach akcji.
  - **PO CO (Value):** Błyskawiczna responsywność sterowania niezależna od opóźnień sieciowych (ping 50–150 ms).
- **User Story 15.2:** Jako inżynier sieciowy, chcę bezalokacyjnego bufora migawek (`SnapshotRingBuffer<T, 256>`) oraz automatycznej rekonsyliacji (Server Reconciliation) z wykładniczym wygaszaniem błędu wizualnego, aby korygować rozbieżności bez brzydkiego 'teleportowania' postaci na ekranie.
  - **PO CO (Value):** Spójność rozgrywki turniejowej, eliminacja desynchronizacji i brak widocznych przeskoków geometrii na ekranie.
- **Status:** `[x] ZROBIONE` (`engine/network`, `tests/NetworkRollbackTests.cpp` — 100% Passed).

---

### Epik 16: Neural Radiance Caching & ML Acceleration [ZROBIONE]
- **User Story 16.1:** Jako inżynier grafiki czasu rzeczywistego, chcę trenować i odpytywać małą sieć neuronową (MLP z kodowaniem harmonicznym NeRF) w locie, aby przewidywać oświetlenie wielo-odbiciowe (Multi-Bounce Indirect Lighting) z pojedynczego trafienia promienia.
  - **PO CO (Value):** Fotorealistyczne, stabilne czasowo globalne oświetlenie bez kosztownego śledzenia dziesiątek odbić promieni na piksel.
- **Status:** [x] ZROBIONE (100% testów w 	ests/NeuralRadianceCacheTests.cpp zaliczonych: weryfikacja 24-wymiarowego kodowania harmonicznego, spadek straty SGD z 0.4742 do 0.0000 i pass NeuralRadianceCachePass w RenderGraphie).

---

### Epik 17: Procedural World Streaming & Virtual Texturing (SVT / Clipmaps) [ZROBIONE]
- **User Story 17.1:** As a Player / Graphics Programmer, I want seamless procedural terrain streaming with zero loading screens using nested concentric clipmaps ( \dots L_5$) and Sparse Virtual Texturing (SVT with a 256-tile physical LRU cache), so that the engine renders infinite landscape detail within a fixed, constant VRAM budget.
  - **PO CO (Value):** Zero vertex popping through continuous C0 geomorphing, zero framerate hitching, and constant bounded VRAM usage.
- **Status:** [x] ZROBIONE (100% test suite in 	ests/WorldStreamingTests.cpp passed: concentric ring spacing doubling 0.5m -> 16m, geomorphing factor continuity [0, 1], and LRU page table eviction upon 256-tile overflow).

---

## 🚀 Sprint Active (In Progress): Epic 18 — AAA Tech Demo Video & Portfolio Showcase

### User Story 18.1: 60-Second AAA Tech Demo Showcase Recording & Technical Benchmark Report
- **As a:** Senior Engine & Graphics Programmer
- **I want:** A comprehensive 60-second Tech Demo demonstration showcasing the 100,000 instance GPU culling test in 1.19 ms, Cascaded Shadow Maps (CSM), Hierarchical Z-Buffer (HZB), DXR Ray Tracing, Voxel Cone Tracing GI, Neural Radiance Caching (NRC), Procedural Terrain Clipmaps, and Deterministic 60 Hz Network Rollback,
- **So that (PO CO):** Studios and recruiters (AAA Lead Engine/Graphics roles) have immediate, tangible, verifiable visual proof of top-tier engine development competence.
- **Acceptance Criteria:**
  - 1080p/60 FPS recording showing real-time scene playback and ImGui docking editor.
  - Automated benchmark telemetry summary displaying CPU vs GPU frametimes (< 16.6 ms).
  - Clean Release build validated under MSVC LTO/LTCG.

---

## 🔮 Sprint Następny i Backlog: Co Nadal Robić? (Product Backlog)

### Epik 17: Procedural World Streaming & Virtual Texture Transcoding
- **Zadanie 17.1:** Strumieniowanie terenu i geometrii z kompresją blokową BC7/ASTC w czasie rzeczywistym. *(PO CO: Brak ekranów ładowania w grach z otwartym światem).*
- **Zadanie 17.2:** Wirtualne teksturowanie (Sparse Virtual Textures / Clipmaps) z natywnym dekodowaniem sprzętowym GPU. *(PO CO: Dowolnie wysoka rozdzielczość tekstur bez wyczerpywania pamięci VRAM).*

### Epik 18: Szlif Portfolio i Nagranie Tech Demo AAA
- **Zadanie 18.1:** Przygotowanie 60-sekundowego materiału wideo Tech Demo prezentującego: stres-test 100 000 instancji w 1.14 ms, kaskadowe cienie, dokowalny edytor ImGui, fizykę Jolta oraz rollback sieciowy. *(PO CO: Bezpośrednia demonstracja umiejętności Senior Graphics/Engine Programmer dla studiów AAA).*

### Epik 19: Dystrybucja i Wieloplatformowość (Linux / Vulkan Packaging)
- **Zadanie 19.1:** Zapewnienie bezproblemowej kompilacji na platformie Linux z wykorzystaniem Clang 18 i natywnego backendu Vulkan 1.3. *(PO CO: Kompatybilność ze Steam Deck oraz serwerami dedykowanymi Linux).*

---

## 🛡️ Definition of Done (DoD) dla Silnika
Każde zadanie i historyjka użytkownika uznawana jest za **Done** tylko wtedy, gdy:
1. Kod kompiluje się z 0 błędami i 0 ostrzeżeniami w MSVC (`/W4`, `/WX`) oraz Clang.
2. Żadna funkcja wywoływana co klatkę (`Tick()`, `Render()`, `Update()`) nie alokuje pamięci na stercie (`new`/`malloc`).
3. Zaimplementowano dedykowany zestaw testów w folderze `tests/` weryfikujący poprawność podsystemu.
4. Cała solucja pomyślnie przechodzi testy automatyczne (`Engine_Tests.exe` zwraca kod 0).
5. Zaktualizowano `IMPLEMENTATION_PLAN.md` oraz niniejszy `AGILE_PRODUCT_BACKLOG.md` w standardzie czystego kodowania UTF-8 bez jakichkolwiek zniekształceń znaków.