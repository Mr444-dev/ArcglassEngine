# Modern 3D Game Engine (Standard 2026) — Karta Projektu i Dokumentacja Architektoniczna (Architecture White Paper)

Niniejszy dokument stanowi oficjalny manifest inżynierski, architektoniczny i strategiczny projektu **MyGameEngine**. Definiuje nadrzędny cel powstania silnika, założenia technologiczne, analizę ryzyk architektonicznych, szczegółowe diagramy przepływu danych oraz strategię budowy profilu zawodowego programisty systemów czasu rzeczywistego (*Engine / Graphics / Systems Programmer*).

---

## 1. Misja, Cel i Uzasadnienie Biznesowo-Inżynierskie

### 1.1. Po co powstaje ten projekt?
Większość hobbystycznych prób napisania własnego silnika kończy się porażką w tzw. **„Pułapce Silnika Gry” (Game Engine Trap)**: programista próbuje od zera stworzyć uniwersalny kombajn imitujący Unity czy Unreal Engine, grzęznąc na lata w pisaniu parserów formatów, edytorów okienkowych czy bibliotek matematycznych, nigdy nie kończąc ani silnika, ani grywalnego projektu.

**Cel tego projektu jest fundamentalnie odmienny:**
> **Zbudować nowoczesny, modularny i wyspecjalizowany silnik 3D oparty o standardy technologiczne roku 2026, który demonstruje opanowanie najtrudniejszych domen nowoczesnej inżynierii gier: programowania GPU-Driven, architektury pamięci wirtualnej, bezstanowego dostępu do zasobów (Bindless) oraz wielowątkowych grafów zadań (Task Graphs).**

Projekt nie ma konkurować z komercyjnymi silnikami ogólnego przeznaczenia. Jest to **„Toy Engine klasy AAA”** – zaprojektowany z myślą o maksymalnej wydajności sprzętowej, zerowym długu technologicznym i bezpośrednim przełożeniu na wymagania rekrutacyjne wiodących studiów deweloperskich na świecie (CD Projekt RED, Ubisoft, Remedy, Techland, Guerrilla Games, id Software).

---

## 2. Nienaruszalne Założenia Architektoniczne (Inviolable Principles)

Silnik powstaje na fundamencie 6 żelaznych reguł inżynierskich:

```
┌────────────────────────────────────────────────────────────────────────┐
│                      6 FILARÓW ARCHITEKTURY 2026                       │
├──────────────────────────────────┬─────────────────────────────────────┤
│ 1. Data-Oriented Design (DOD)    │ Zwarte tablice struktur (SoA),      │
│                                  │ brak wirtualnych hierarchii scen.   │
├──────────────────────────────────┼─────────────────────────────────────┤
│ 2. Zero Heap Allocation in Loop  │ Zakaz malloc/new w pętli gry.       │
│                                  │ Cała pamięć ramki z FrameAllocator. │
├──────────────────────────────────┼─────────────────────────────────────┤
│ 3. GPU-Driven Visibility         │ Frustum & HZB Occlusion Culling     │
│                                  │ na GPU; CPU wydaje 1 indirect call. │
├──────────────────────────────────┼─────────────────────────────────────┤
│ 4. Bindless First (SM 6.6)       │ Koniec z bindowaniem slotów.        │
│                                  │ Globalna sterta indeksowana ID.     │
├──────────────────────────────────┼─────────────────────────────────────┤
│ 5. Render Graph Orchestration    │ Deklaratywne pasy, automatyczne     │
│                                  │ bariery zasobów i memory aliasing.  │
├──────────────────────────────────┼─────────────────────────────────────┤
│ 6. Strict Runtime/Editor Split   │ Czysta gra nie wie o istnieniu      │
│                                  │ edytora ani biblioteki Dear ImGui.  │
└──────────────────────────────────┴─────────────────────────────────────┘
```

---

## 3. Na Co Bezwzględnie Zwracać Uwagę (Pułapki i Mitygacje)

Podczas implementacji silnika 3D istnieją krytyczne obszary, w których drobny błąd architektoniczny niszczy płynność lub stabilność całego systemu:

### A. Blokady i Wyścigi na Magistrali Pamięci CPU (Cache Contention)
- **Problem:** Tradycyjne wzorce OOP (`class Actor` z 50 polami i wskaźnikami do komponentów) powodują, że procesor spędza 70% czasu na czekaniu na sprowadzenie danych z pamięci RAM do pamięci podręcznej $L1/L2/L3$ (*cache misses*).
- **Zasada:** Obiekty o tym samym typie żyją w ciągłych tablicach. Aktualizacja macierzy transformacji nie używa rekursji drzewiastej, lecz jednej liniowej pętli po płaskim buforze posortowanym w kolejności DFS (*Depth-First Search*).

### B. Przełączanie Stanów i Synchronizacja GPU (Pipeline Stalls & Barriers)
- **Problem:** Błędnie wstawione bariery zasobów (`vkCmdPipelineBarrier2` / `D3D12_RESOURCE_BARRIER`) zmuszają kartę graficzną do opróżnienia całego potoku wykonawczego (*GPU pipeline bubble*), degradując wydajność o połowę.
- **Zasada:** Programista nigdy nie wstawia barier manualnie w kodzie renderowania! O kolejności przejść stanów decyduje **Render Graph** na podstawie analizy zależności wejść i wyjść pasów.

### C. Eksplozja Permutacji Shaderów (Shader Permutation Hell)
- **Problem:** Dodawanie kolejnych makr `#ifdef HAS_NORMAL_MAP`, `#ifdef HAS_SHADOWS` prowadzi do generowania setek tysięcy wariantów Pipeline State Object (PSO), powodując wielogodzinne kompilacje i przycięcia ekranu (*hitching*).
- **Zasada:** Dzięki architekturze **Bindless** materiały różnią się danymi w buforze, a nie strukturą shadera. Wszystkie powierzchnie PBR współdzielą ten sam potok shadera pikseli.

### D. Wycieki Pamięci i Fragmentacja Sterty
- **Problem:** Dynamiczne tworzenie i niszczenie stringów, wektorów i obiektów w pętli gry rozrywa przestrzeń adresową sterty Windows/Linux, prowadząc do nieprzewidywalnych spadków wydajności.
- **Zasada:** W pętli klatki alokacja stertowa jest traktowana jak błąd kompilacji. Pamięć na dane tymczasowe pobierana jest wyłącznie z superszybkiego `FrameAllocator` (resetowanego co klatkę jednym przestawieniem wskaźnika).

---

## 4. Szczegółowy Stos Technologiczny (Technology Stack 2026)

| Kategoria | Rekomendowany Wybór | Rola w Silniku | Dlaczego ten wybór? (Uzasadnienie) | Odrzucona Alternatywa |
| :--- | :--- | :--- | :--- | :--- |
| **Język i Standard** | **C++20 / C++23** | Rdzeń silnika | Koncepcje (`concepts`), moduły, `std::format`, `constexpr`/`consteval`, brak narzutu RTTI. | C++14/17 (przestarzałe), Rust (trudna integracja z middleware C++). |
| **Build System** | **CMake 3.28+ + Ninja** | Generowanie projektów | Standard de facto w branży gier AAA; błyskawiczna kompilacja przyrostowa z generatorem Ninja. | Autorskie skrypty batch/python, Makefiles. |
| **Kompilator** | **MSVC v143+ / Clang 18+** | Kompilacja kodu | Doskonała optymalizacja wektorowa SIMD, pełna zgodność z Windows SDK i DirectX 12 Agility. | Przestarzałe wersje GCC. |
| **Niskopoziomowy RHI** | **DirectX 12 Agility SDK** | Abstrakcja GPU | Największe zapotrzebowanie rynkowe w studiach PC/Konsolowych; natywne wsparcie Shader Model 6.6+ i DXR. | DirectX 11 / OpenGL (przestarzałe interfejsy blokujące asynchroniczność). |
| **Alokator Pamięci GPU** | **D3D12MA / VMA** | Zarządzanie VRAM | Standard branżowy (AMD/NVIDIA); bezbłędna subalokacja buforów i tekstur w dużych stertach VRAM. | Samodzielne pisanie buddy allocatora dla VRAM (ogromny koszt czasowy). |
| **Matematyka 3D** | **DirectXMath / GLM** | Wektory, macierze, SIMD | Rejestry wektorowe procesora (SSE2/AVX2/NEON); zero narzutu na operacjach macierzowych. | Własna klasa Vector bez SIMD (katastrofalna wydajność matematyki). |
| **Architektura Danych** | **EnTT (Sparse Set ECS)** | Zarządzanie encjami | Płaskie tablice spakowane; $O(1)$ dla dodawania/usuwania komponentów; brak fragmentacji archetypów. | Tradycyjne hierarchie klas OOP (pointer chasing). |
| **Fizyka 3D** | **Jolt Physics (v5+)** | Ciała sztywne, ragdoll, kolizje | Stworzony dla *Horizon Forbidden West*; wielowątkowa architektura zorientowana na Task Graph. | PhysX 5 (cięższy narzut), Bullet (przestarzały solver). |
| **Format Modeli** | **fastgltf + custom .cmesh** | Import geometrii | Parsowanie glTF oparte o instrukcje SIMD (10x szybsze niż Assimp); gotowanie do natywnych blobów. | Assimp (wolny, generuje nieoptymalne struktury pamięci). |
| **Optymalizacja Siatek** | **meshoptimizer** | Podział na meshlety | Generowanie klastrów dla Mesh Shaderów, optymalizacja pamięci podręcznej wierzchołków GPU. | Ręczny podział geometrii. |
| **Animacja Szkieletowa** | **ozz-animation** | Sampling i dekompresja | Wzorcowy kod SIMD dla krzywych animacji; idealny do przekazania do Compute Skinning shadera. | Własny sampling animacji (podatny na błędy numeryczne). |
| **Interfejs Edytora** | **Dear ImGui (Docking)** | UI narzędziowe | Błyskawiczne prototypowanie paneli diagnostycznych; zerowy narzut w trybie Release gry. | Qt (monolityczny, ciężki, trudny w integracji z RHI). |
| **Profilowanie** | **Tracy Profiler** | Analiza CPU/GPU/Pamięć | Najlepszy profiler w branży gier; precyzyjne mikrosekundowe strefy CPU i GPU w czasie rzeczywistym. | std::chrono wypisywane w konsoli. |

---

## 5. Inżynierskie Diagramy Architektury

### Diagram 1: Jednokierunkowy Graf Zależności Modułów (Strict DAG)
Żaden moduł niższy nie ma prawa wiedzieć o istnieniu modułu wyższego. Zapewnia to 100% modularność i możliwość niezależnego testowania każdego podsystemu.

```text
                                  ┌──────────────────┐
                                  │  Engine.Editor   │ (Narzędzia, ImGui, Panele)
                                  └────────┬─────────┘
                                           │
                                  ┌────────▼─────────┐
                                  │  GameRuntime     │ (Czysty launcher gry)
                                  └────────┬─────────┘
                                           │
        ┌──────────────────────────────────┼──────────────────────────────────┐
        │                                  │                                  │
┌───────▼────────┐                ┌────────▼────────┐                ┌────────▼────────┐
│Engine.Renderer │                │ Engine.Physics  │                │Engine.Animation │
│(RenderGraph,   │                │(Jolt Wrapper,   │                │(ozz-animation,  │
│ GPU-Driven, PBR)│                │ Character Ctr)  │                │ Compute Skinning│
└───────┬────────┘                └────────┬────────┘                └────────┬────────┘
        │                                  │                                  │
        └──────────────────────────────────┼──────────────────────────────────┘
                                           │
                                  ┌────────▼─────────┐
                                  │   Engine.World   │ (EnTT ECS, Płaski Bufor DFS)
                                  └────────┬─────────┘
                                           │
                                  ┌────────▼─────────┐
                                  │  Engine.Assets   │ (VFS, Cooker, Binary Formats)
                                  └────────┬─────────┘
                                           │
                                  ┌────────▼─────────┐
                                  │    Engine.RHI    │ (D3D12, Vulkan, Sterty Bindless)
                                  └────────┬─────────┘
                                           │
                                  ┌────────▼─────────┐
                                  │ Engine.Platform  │ (SDL3, Okno, Bufor Wejścia)
                                  └────────┬─────────┘
                                           │
                                  ┌────────▼─────────┐
                                  │   Engine.Core    │ (Allocators, JobSystem, Math, Log)
                                  └──────────────────┘
```

---

### Diagram 2: Cykl Życia Ramki Silnika (CPU vs GPU Frame Timeline)
Dzięki podwójnemu buforowaniu i asynchronicznym kolejkom, procesor przygotowuje klatkę $N+1$, podczas gdy karta graficzna renderuje klatkę $N$.

```text
CPU MAIN THREAD:
├── [Poll Platform Events & Input]
├── [JobSystem: Kick Physics Step (60 Hz Fixed Tick)]
├── [JobSystem: Kick Animation Pose & Matrix Update]
├── [Transform System: Linear Vectorized DFS Flattening]
├── [RenderGraph: Declare Passes & Compile Barriers]
├── [GPU Culling Dispatch & ExecuteIndirect Recording]
└── [Submit Command Lists to GPU Queue] ──► [Present / SwapBuffers]

WORKER THREADS (Job System - Work Stealing):
├── Thread 1: ──► [Physics Narrow-phase Island 1] ──► [Skinning Compute Task]
├── Thread 2: ──► [Physics Narrow-phase Island 2] ──► [BVH Refit Task]
└── Thread 3: ──► [Animation Curve Sampling]     ──► [VFS Async File Decompress]

GPU TIMELINE (Concurrent Execution):
├── Depth Prepass (Early-Z)
├── Hierarchical-Z (HZB) Pyramid Generation (Compute Shader)
├── Two-Phase Occlusion Culling (Compute Shader -> Indirect Draw Buffer)
├── G-Buffer / Visibility Buffer Rasterization
├── Clustered Light Assignment (Compute Shader)
├── PBR Lighting & Shadows Resolution
├── Post-Processing (TAA, Tonemapping ACES)
└── SwapChain Present
```

---

### Diagram 3: Architektura Pamięci Silnika (Memory Subsystem Layout)
Silnik całkowicie eliminuje systemowy `malloc` na rzecz specjalizowanych alokatorów pamięci wirtualnej:

```text
                           VIRTUAL MEMORY (OS)
             (VirtualAlloc MEM_RESERVE: 4 GB przestrzeni adresowej)
                                    │
       ┌────────────────────────────┴────────────────────────────┐
       ▼                                                         ▼
LINEAR FRAME ALLOCATOR                                  FIXED-BLOCK POOL ALLOCATOR
(Pamięć Tymczasowa Ramki)                               (Pamięć Trwała o Stałym Rozmiarze)
       │                                                         │
 ┌─────┴──────────────────┐                               ┌──────┴──────────────────┐
 ▼                        ▼                               ▼                         ▼
Arena A (Klatka Parzysta) Arena B (Klatka Nieparzysta)    Pool: ECS Nodes           Pool: BVH Nodes
- Reset wskaźnika: O(1)   - Reset wskaźnika: O(1)         - FreeList O(1)           - FreeList O(1)
- Zero fragmentacji       - Zero fragmentacji             - Zero alokacji OS        - Wyrównanie 64B
```

---

### Diagram 4: Potok Renderowania GPU-Driven (Brak narzutu CPU na Draw Calle)
Tradycyjny silnik wysyła 5 000 wywołań `DrawIndexed` z CPU. Nowoczesny silnik wysyła **1 wywołanie obliczeniowe i 1 wywołanie pośrednie**:

```text
[CPU]
1. Prześlij 1 płaską tablicę: StructuredBuffer<GPUInstanceData> (Macierze, AABB, MaterialID)
2. Wywołaj Dispatch(ComputeShader: GPUFrustumAndOcclusionCulling)
3. Wywołaj ExecuteIndirect(IndirectDrawCommandBuffer)
        │
        ▼
[GPU COMPUTE SHADER: Culling Pass]
├── Sprawdź czy AABB przecina płaszczyzny Frustum kamery
├── Zrzutuj AABB na bufor HZB (Hierarchical-Z Buffer)
├── Jeśli obiekt zasłonięty -> Odrzuć trójkąty w locie
└── Jeśli widoczny -> Atomowo dopisz do bufora pośredniego:
    InterlockedAdd(DrawCounter, 1, instanceIndex);
    IndirectDrawArgs[instanceIndex] = CalculateDrawArgs();
        │
        ▼
[GPU RASTERIZER]
└── Karta graficzna sama rysuje wyłącznie widoczne obiekty bezpośrednio z pamięci VRAM!
```

---

## 6. Strategia "Build vs Buy" & Wartość w Portfolio/CV

Podczas technicznych rozmów kwalifikacyjnych rekruterzy natychmiast wychwytują różnicę pomiędzy bezmyślnym łączeniem bibliotek a świadomym projektowaniem systemów:

```
┌────────────────────────────────────────────────────────────────────────┐
│                   JAK ARGUMENTOWAĆ WYBORY NA ROZMOWIE                  │
├───────────────────────────────┬────────────────────────────────────────┤
│ Co zrobiono SAMODZIELNIE:     │ Czego to dowodzi:                      │
│ - Własny menedżer pamięci     │ "Rozumiem działanie pamięci wirtualnej,│
│   (VirtualAlloc/Linear/Pool)  │  linii cache i eliminacji fragmentacji"│
│ - Własny Job System (Lock-free│ "Znam mechanikę atomików C++, barier   │
│   Work-Stealing Deque)        │  pamięci std::memory_order i wątków"   │
│ - Render Graph (FrameGraph)   │ "Potrafię zarządzać stanami zasobów    │
│                               │  GPU i aliasowaniem pamięci VRAM"      │
│ - GPU-Driven Culling Pipeline │ "Znam nowoczesne podejście do potoku   │
│                               │  graficznego bez dławienia CPU"        │
├───────────────────────────────┼────────────────────────────────────────┤
│ Co zintegrowano (Middleware): │ Czego to dowodzi:                      │
│ - Jolt Physics                │ "Nie marnuję miesięcy na pisanie gorszej│
│                               │  wersji solvera LCP; potrafię integrować│
│                               │  złożone middleware klasy AAA"         │
│ - fastgltf & meshoptimizer    │ "Rozumiem nowoczesne formaty i podział │
│                               │  siatek pod kątem Mesh Shaderów"       │
│ - Dear ImGui (Docking)        │ "Wiem, jak budować ergonomiczne        │
│                               │  narzędzia dla projektantów gier"      │
└───────────────────────────────┴────────────────────────────────────────┘
```

---

## 7. Wzorcowy Opis Projektu do Sekcji "Projekty" w CV

Gdy projekt osiągnie Fazę 7, poniższy wpis w CV gwarantuje najwyższy wskaźnik zaproszeń na rozmowy techniczne:

```markdown
**Autorski Nowoczesny Silnik Gier 3D (C++20 / DirectX 12 Agility SDK / Vulkan)**
- Zaprojektowano dwufazowy potok renderowania napędzany przez GPU (GPU-Driven Rendering) wykorzystujący bufor HZB do selekcji okluzji w Compute Shaderze oraz bezstanowe wywołania ExecuteIndirect, redukując narzut CPU do poniżej 5 wywołań rysowania na klatkę.
- Zaimplementowano bezstanowy potok materiałowy (Bindless Rendering) w standardzie Shader Model 6.6, eliminujący zmiany stanów stert deskryptorów w trakcie ramki.
- Zbudowano autorski Render Graph (FrameGraph) z automatycznym sortowaniem topologicznym pasów, deterministycznym barierowaniem zasobów GPU oraz aliasowaniem pamięci VRAM (Placed Resources).
- Zaprojektowano menedżer pamięci wirtualnej z podwójnie buforowaną areną ramki (Frame Allocator) i alokatorami puli O(1), całkowicie eliminujący alokacje na stercie w pętli gry.
- Zaimplementowano wielowątkowy system zadań (Task Graph) oparty o kolejki kradzieży pracy (Work-Stealing) bez blokad synchronizacyjnych.
- Zintegrowano fizykę Jolt Physics, animację ozz-animation oraz dokowalny edytor narzędziowy Dear ImGui.
```

---

## 8. Standardy Kodowania i Kultura Inżynierska

1. **Język:** C++20. Żadnych archaicznych konstrukcji typu `NULL`, `typedef` czy surowych tablic w stylu C `int arr[10]`.
2. **Formatowanie:** Ścisłe reguły formatowania zgodne z dołączonym plikiem [.clang-format](file:///c:/Users/rusin/Documents/MyGameEngine/.clang-format).
3. **Konwencje Nazewnictwa:**
   - Typy, Klasy, Struktury: `PascalCase` (np. `FrameAllocator`, `RHICommandList`).
   - Metody i Funkcje: `PascalCase` (np. `Allocate()`, `ExecuteIndirect()`).
   - Zmienne lokalne i parametry: `camelCase` (np. `sizeInBytes`, `targetQueueIndex`).
   - Pola prywatne składowe klas: przedrostek `m_` (np. `m_capacity`, `m_freeList`).
   - Zmienne statyczne globalne: przedrostek `g_` (np. `g_logMutex`).
   - Zmienne lokalne wątku: przedrostek `t_` (np. `t_workerIndex`).
4. **Czystość Przestrzeni Nazw:** Bezwzględny zakaz stosowania `using namespace std;` w plikach nagłówkowych (`.h`).
5. **Kompilacja:** Projekt musi zawsze kompilować się z flagami `/W4` (MSVC) oraz `-Wall -Wextra` (Clang/GCC) z **0 błędami i 0 ostrzeżeniami**.

---

*Dokument ten stanowi przewodnik dla całej implementacji silnika. Każda decyzja kodowa musi być z nim w 100% spójna.*
