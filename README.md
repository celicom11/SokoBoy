# SokoBoy
Yet another Sokoban Solver?? Oh boy...

## Motivation
While playing around with a few Sokoban puzzles from [SokobanOnline](https://www.sokobanonline.com/) I was stuck with one of them and struggled for a few days. As I could not find neither a solution nor any hints online, I decided to ~~kindly ask ChatGPT~~ find a solution computationally. Being a ~~Signore :tomato:~~ senior software developer for years, the idea to quickly grab_build_run some existing code to smash that annoying puzzle seemed to be an easy walk. As I am not a Java fan and I wanted just something fast-N-edible, I took a ~~BigMac~~ C++ GitHub [SokobanSolver](https://github.com/SanGuillao/SokobanSolver) and ... found that even if it could solve anything, it only produced an "infinite" list of stages instead of a short and neat moves/pushes solution. I decided quickly to fix it and maybe add a few other things and... my long voyage began. 
It comes that solving a Sokoban puzzle in general is a known, deeply learned and quite a complicated matter (See "Solving Sokoban" below). After spending few weeks of my free-time, it has left so little from the original SokobanSolver (though I give all credits to SanGuillao for good DFS, BFS and A* search basic C++ implementations) and the application does what I wanted from the beginning- it solves small-medium Sokoban puzzles and outputs the sequence of pushes from the starting to the ending position, I have decided to make it public hoping some Sokoban researchers/funs could find it useful somehow.

## Description
**SokoBoy** application is a console application, developed with C++/14, MS VisualStudio 2019. It takes one or more Sokoban puzzles (text files in [XSB format](http://www.sokobano.de/wiki/index.php?title=Level_format) ) and tries to solve them with one of the known search algorithm mentioned above: BFS, DFS and AStar. It also uses various parameters from the configuration file. The output is a text file containing a list of stages (in XSB format) to represent a squence of box pushes (NOT robot moves/steps, which is a bit more complicated!). The SokoBoy has limits on the puzzles field's size/spaces and boxes numbers - after all, its just a SokoBoy :boy: !
**Note:** the generated positions/nodes are often called **stages**.
## How to Build
 ### Windows/MSVC
Just open the SokoBoy.vcxproj project and build. There is pure C++/17, STL code, no dependencies. Code uses Unicode and can be built targeting both x32/x64 arch.
### Linux/Mac
Recently Windows specific API was removed from _ReadAllFiles method to so the code should be cross-platform ready - not tested though.
## How to Use
 ### Puzzles
  After building the SokoBoy, prepare or download a text file (usually with a txt/xsb/soko extension) with a **single** puzzle in correct XSB format.
 ### Limitations
  - Puzzle field can have **maximum 16 cells** in either X/Y directions (including borders!).
  - Number of free (not walls) spaces **cannot exceed 64**.
  These limitations are impeded by the current implementation - sorry for the inconvenience. Only a few puzzles from the known large sets like a Microban, Original, etc. fit these limits. However, a great number of interesting puzzles from great SokobanOnline's collections like IonicCatalyst***, etc. will perfectly fit.
 ### Configuration
  Use SokoBoy.cfg from this repo as a template. The config file should be located in the folder where SokoBoy app runs. Config contains: 
  - _Comments_: any line started with ';' may contain arbitrary text. Comments are also allowed after Key=Value pairs.
  - _PuzzlePath_ ={full or relative path to one or more puzzle files}. Wildcards */? are allowed in accordance to FindFirstFile/FindNextFile Win API.
  - _Search_ ={DFS|BFS|AStar} . For easy puzzles use BFS which gives an optimal/minimal push count solution. For complicated, use AStar - it creates a sub-optimal solution within a reasonable time (but not always!). DFS is not recommended/not working well yet.
  - _RSM_Depth_ = 0(default/auto) or a positive number that defines **a depth of the Reverse-Pull-Tree** (aka a radius of the **Reverse Sphere**) which is used by the AStar/DFS algorithms. _RSM_Depth_ = 1 is the best for the most of the levels. For complex puzzles though it could be increased to get some reasonable number of pull/reversed stages, but the 10,000+ nodes will slow down the search.
  - _RSM_GBRelax_ = {0|1(default)|2}. Number of GoalBox removals for LBE heuristic pre-calculations. 2 is NOT yet implemented, 0 - is not recommended, but it is kept for testing reasons.
  - _DFS_MaxDepth_ = 0(default. no limit), or a positive limit/cutoff for a DFS tree depth. Should be slightly above presumed minimal/optimal number of pushes in the puzzle solution. Again, at the moment DFS is not ready to compete with BFS/A* in most cases.
  - Rpt_Sol={3|2|1|0} - how to output solution file. If solution is found, the output file with contain seriies of Levels (if 1 or 3) and/or LURD (if 2 or 3) string. The output file is written to the puzzle's location/folder as {puzzle_file_name}_{Algorithm}.txt .
  - _Rpt_SQInc_ = 0(no reporting} or positive number N (~1000 is recommended) for reporting the current node/stage and its distance(**DST**), the queued/closed nodes/stages of the search process, etc. on every moment the stages queue size has changed by N stages.
See the details/terms in "Sokoban: Solving Techniques,..." and "Notes on Implementation" section.
 ### App's Output/Reporting
        Solving GS11065.xsb with AStar
        Initializing ...
        RSphere of radius 12 has 5915 nodes.
        ▓▓▓▓▓▓▓▓▓▓▓▓▓
        ▓▓▓▓▓▓▓▓    ▓
        ▓▓▓▓▓▓▓▓ ▓◦▓▓
        ▓ ■ ■☻■ ■■◦ ▓
        ▓ ▓◦□◦▓◦◦◦□ ▓
        ▓ ■ ■ ◦■■▓◦ ▓
        ▓           ▓
        ▓▓▓▓▓▓▓▓▓▓▓▓▓
        
        ▓▓▓▓▓▓▓▓▓▓▓▓▓
        ▓▓▓▓▓▓▓▓    ▓
        ▓▓▓▓▓▓▓▓ ▓◦▓▓
        ▓ ■ ■ ■ ■■◦ ▓
        ▓ ▓◦□◦▓□◦◦□ ▓
        ▓  ■ ■☺ ■▓◦ ▓
        ▓           ▓
        ▓▓▓▓▓▓▓▓▓▓▓▓▓
        DST:  QS: 1000 ALL: 682 DEPTH: 5 DDLs: 18
        ▓▓▓▓▓▓▓▓▓▓▓▓▓
        ▓▓▓▓▓▓▓▓    ▓
        ▓▓▓▓▓▓▓▓ ▓◦▓▓
        ▓ ■ ■ ■  ■◦ ▓
        ▓ ▓◦□◦▓◦◦□◦ ▓
        ▓  ■ ☻□■■▓□ ▓
        ▓           ▓
        ▓▓▓▓▓▓▓▓▓▓▓▓▓
        DST:  QS: 2007 ALL: 1687 DEPTH: 6 DDLs: 30
        ▓▓▓▓▓▓▓▓▓▓▓▓▓
        ▓▓▓▓▓▓▓▓    ▓
        ▓▓▓▓▓▓▓▓ ▓◦▓▓
        ▓ ■   ■  ■◦ ▓
        ▓ ▓□□□▓□□□☺ ▓
        ▓    ■◦  ▓□ ▓
        ▓           ▓
        ▓▓▓▓▓▓▓▓▓▓▓▓▓
        DST: 31 QS: 3012 ALL: 2166 DEPTH: 17 DDLs: 31
        ▓▓▓▓▓▓▓▓▓▓▓▓▓
        ▓▓▓▓▓▓▓▓    ▓
        ▓▓▓▓▓▓▓▓ ▓◦▓▓
        ▓      ☻■■◦ ▓
        ▓ ▓□◦□▓□◦□□ ▓
        ▓ ■ ■ ◦ ■▓□ ▓
        ▓           ▓
        ▓▓▓▓▓▓▓▓▓▓▓▓▓
        DST: 21 QS: 4012 ALL: 3024 DEPTH: 41 DDLs: 33
- DST: Estimated distance of the current stage to the RSphere stages.
- QS: Number of stages in the queue
- ALL: Number of closed (processed) stages.
- DEPTH: depth of the current stage/node.
- DDLs: count of the Dynamic DeadLocks
## Puzzles directory
Contains various puzzles/levels taken from public sources and which are used for the Sokoboy testing. Current version can quickly solve most of the levels there. 
- Hard one, for example, is *6170_moves_more* from https://sokoban-max-moves.herokuapp.com/, but with recent performance improvements it took 1135 seconds to find 1474 pushes to solve it.
- Unslovable:
  - **sasquatch_iii_12.xsb**:  it contains quite "unpleasand" goals configuration at the top, which require very strict order of filling. Otherwise a non-static,non-PI-Corral deadlock gets created which is not recognized by the Sokoboy. To be resolved with a "semi-fixed goals" implementation (see below).
  - **MF8_196th_Mainv5.xsb**: simplified version of the main contest level and still is too hard for the Sokoboy. To be investigated how to improve algorithms.  
- Sub-dir **sokhard** contains a subset of the Lee J. Haywood collection https://www.sokobanonline.com/play/web-archive/lee-j-haywood/sokhard that fits Sokoboy's size/number of boxes restricitons. The most hard/time-consuming there are: 112-Catalina(1307s), 110-Antonia(447s), 109-Terra (144s), 121-Gerrilyn (138s), 120-Heulwen (134s). 112-Catalina to be investigated.
## How to Contribute
As for any new/not-well-tested software, any suggestions, requests, bug findings, etc. are **VERY WELCOME**! Modifying/extending current code could be done with the standard GitHub forking/pulling [workflow](https://docs.github.com/en/get-started/exploring-projects-on-github/contributing-to-a-project).

## Solving Sokoban: Theory/Solutions 
### Overview
Imho, the most fundamental and comprehensive work I have found so far was the doctoral dissertation of Mr. Junghanns [[Jun99]](#1). It is referenced virtually in every Sokoban related paper and it covers various aspects of the problem. I also liked Masters Thesis of Timo Virkkala that contains a long list of related papers [[Virkk]](#2). Among the websites, I regularly visited Sokoban Wiki [[sbowiki]](#3) where one can find a good list of known Sokoban solvers, description of the approaches/techniques, challenges and test results.       
### Terms/Key techniques
Here is a list terms and the key approaches names with (*)/comments if they are used in the SokoBoy. The detailed of description of these terms can be found in the links/papers provided below:
- Moves/Pushes Parity (invariant).
- Breadth First Search/BFS; Depth First Search/DFS, Iterative Deepening Depth-First Search/IDDFS.  (*, IDDFS is not implemented in SokoBoy)
- Tunneling/Macromoves (* SokoBoy implements Tunneling as described in [[sboYass]](#4) and other sources.
- Goal Parking Order/GPO.
- Push/Move Ordering, Inertia Move. 
- Deadlock/Deadlocks Table Database. (*, SokoBoy builds DeadLocks dynamically on top of static deadlocks using PI-Corrals)
- Forward, Reverse and Bidirectional Solving. (*, SokoBoy has kind of Bidirectional/RBFS approach for A** search).
- PI-Corrals. (*, SokoBoy has elaborated PI-Corrals merging)
- Relevance Cuts. [[Jun99]](#1)
- Lower Bound Estimation (of the distance between Sokoban stages)/LBE, [[Jun99]](#1),[[pdb]](#5)) (*, SokoBoy has a custom LBE heuristic).
- Hungarian method for Assignment/Graph perfect bipartite problems (*, Sokoboy uses adapted impl of [[AL]](#6) algorithm for LBE/A**).
- Perfect Matching - (*Sokoboy uses standard Kuhn's algorithm).
- Nearest Neighbor Search(NNS) - again, brute-force algorithm for finding nearest one of the precalculated Fixed-Goals configuration nearest (in Hamming distance) to the given stage. TODO:  Locality Sensitive Hashing/Min-Hash, etc.
  
## Notes on SokoBoy implementation
### General
- **Coding Style/OOP**. SokoBoy is a single-threaded console app, implemented with C++/STL14. It has a mix of C++ classes/C-POD structures for optimal memory usage. Old-school Hungarian notation is in use (so please stick to it if you want to modify/extend the code 😉).
- **Sokoban Field/Search Tree encoding**. Application uses compact Stage structure which stores robot's position, weight, parent's index and boxes position as int64/64 bit integer bit mask. While this implies the limitation of having maximum 64 free cells, it provides quite a memory saving PLUS super-fast check for deadlocks detection as a bitwise-AND test. The search tree is divided by two parts: processed (closed) and to-be-processed nodes (aka **stages**). Closed nodes are stored in the flat vector (can be also a list) container, so the parent node is encoded by its *fixed* index in the container. Because of this approach, the container is not an unordered_set ans so the check if the new stage has been met already takes a time leinear to the "closed" container size = not perfect for complex puzzles.
### Search Impl. 
Sokoboy has the same/single (!) loop for all BFS, DFS and AStar searches. The difference of how to pick the next stage/node to process are provided by the hidden implementation of the abstract IStageQueue interface. Internally, an STL's deque is used for BFS, the priority_queue for the AStar and the vector/stack for the DFS. Recalculation of the stage’s weight (estimated distance to the solution) is done for AStar/DFS when node is pushed into the container. 
Another important note is that Sokoboy searches only for min-pushes optimized solution, **not min-moves!**. So, for every stage it searches for all reachable cells (aka Corral0), where the robot can push anything.
### Static Deadlocks
At the beginning, I tried to find/prepare a decent database of all possible (at least) 4x4 size Deadlocks before realizing it would be rather too big and checking every stage against it would noticeably slow down the searching. Next attempt was to use pre-calculated 3x3 static deadlocks on top of 2x3 deadlock tables. Currently, Sokoboy uses a simple algorithm for a detection of "Chain of Locked Boxes" (no boxes can be pushed to a valid square). Static Deadlock tables are removed.
### Dead Cells/Walls
- The "dead wall" is a wall with two blocked ends:  
  `#        #`  
  `####...###`  
- If there are no any goal/storage cells near that "wall" all cells are treated/saved as "dead" - no boxes can be placed there.
- If there is a 1+ goal, the algorithm still remembers these "walls" and prohibits pushing to the wall's cells more boxes than the number of the goals there. **Note:** This only partially compensates the absence of the dynamic "dead squares" recalculation done by RollinStone's solver when a box reaches the "fixed" goal - tobe added to Sokoboy soon.    
### PI-Corrals/Merging
PI-Corral's pruning is a great idea introduced in YASS solver. While it is clear how it should work, I feel it is not well formalized/verified especially when it comes to merging the neighbor PI-Corrals. I could not understand the pseudo code in [[sboYass]] and could not find an answer on how to treat the box on the PI-Corral's border which cannot be moved immediately, because it is blocked by another box. So, the job was done and Sokoboy can well merge PI-Corrals when and only when it is necessary. The algorithm is yet to be verified/tested.  
### Dynamic Deadlocks
As mentioned above for each stage, the Search first obtains a Corral0 to get all "reachable cells". The next step is to check for the existence of the PI-Corral (merged with neighbors if needed). Then, if the robot could not push a single box without being deadlocked, the current PI-Corral is treated as a **Dynamic Deadlock/DDL**. The location of the boxes is saved by the DeadlockManager and all next stages are checked against the presence of this "dead" PI-Corral (by simple bitwise AND check, as mentioned above). 
### LBE Heuristic/"Reverse Sphere"
The "lower bound estimation" heuristic is, imho, the crucial part of any AStar-like algorithms for the simple reason - if it could well approximate the distance between the current and the goal stages, it could find the solution almost immediately by picking the next best stage from the current(or stages - not big difference) and approaching to the goal stage by the linear time or so. That's why it is OK to spend a considerable time trying to improve the calculation of the LBE function at the static stage analysis (before pushing any box). See more details in [[Jun99]](#1) and [[pdb]](#5).
SokoBoy uses a kind-of a "bidirectional reverse search" approach. That is, 
- when the puzzle gets loaded, after basic initializations, the CRStages object runs reverse BFS/RBFS to pull boxes from the goal stage (from all possible corrals!) to build the tree of some heuristic (__or  *RSM_Depth* config value__) depth. The leaf nodes/stages of this tree are called an **Reverse Sphere/RSphere**. [**Note:** the term "sphere"/circle come from the fact that both BFS/RBFS generates all possible nodes with **exact** distances from the starting stage (center). Also, it is not possible to reach the "center" stage from some outer stage without bypassing at least one stage on the surface of the "RSphere"].   
Then, for each RSphere stage/for each box, the pull distance from the box to all free cells is obtained. We do not discard all other boxes (like a "Simple Lower Bound"/Manhattan distance described in [[Jun99]](#1) and others), so they may influence or even block the given box to reach some free cells. To relax this rather strong restriction, the distance measurement is repeated by removing the _RSM_GBRelax_ number (currently 0 or 1 only) of "movable" boxes and updating the distances to the "unreachable" cells in the first measurement with the new one with some "penalty" (=4 for now). It presumes, the box could be pushed/pulled way to allow the current box to reach a given "free cell" and then moved back.
This heuristic, technically, cannot be called an LBE as the real push/pull distance can be less than this estimation. The rationale for this approach is: 
a. use the influence of the neighbor boxes instead of analyzing "Linear Conflicts" etc. (see [[Jun99]](#1))
b. if it is possible to somehow push all boxes except one from the "current" stage  to the RSphere's box positions, such pre-calculated "pull distance" becomes a "real push distance" for that last box.
- at the runtime/search phase, the "weight" of the current stage is re-calculated after every push as a minimal LBE to one of the RSphere stages using
  1. pre-calculated above pull distances
  2. Hungarian method as described in [[Jun99]](#1)/[[Virkk]](#2) to calculate a real minimal distance for a set of boxes/goals.
If/when the "weight" becomes 0, the search stops as the full path to the "goal stage" can be constructed as a path from the initial to current "0-weight" stage + path from that "0-weight" to the "goal stage" (= RSphere center).   
### Fixed Goals/ Parking Order Analysis
Quite a complicated analysis of stages with boxes that are "fixed"in final/G squares. It worth of a separate documentation/paper.
### Next Steps/TODOs
1.(*DONE* with "Fixed Goals") As mentioned above, I am to add the precalculation/usage of the "dynamic dead cells" on occasions when goal squares become occupied and "fixed" like this:  
`#* #*    *#`  
`_# **   #*_`  
2. "Relevance Cuts" to be designed/implemented.
3. Limits of 16x16, 64 free spaces to be increased to 20x20, 128 spaces.
4. Fix DFS.
5. Document Fixed Goals. 
6. Extend FixedGoals to use: 
 - "semi-fixed goals" - goals with limited moves around the G pos (f.e. near the DeadWall )
 - replace Reverse Sphere with adding a "zero-FGs" (that is "no-fixed-goals" stage) to Fixed Goals 

## Sokoban References/Links
1. <a id="1">[Jun99]</a> Junghanns, A., *"Pushing the Limits: New Developments in Single-Agent Search."* Doctoral dissertation, University of Alberta, Edmon-
ton, Alberta, Canada, 1999.
2. <a id="2">[Virkk]</a> Timo Virkkala, *Solving Sokoban Masters Thesis*, https://sokoban.dk/wp-content/uploads/2016/02/Timo-Virkkala-Solving-Sokoban-Masters-Thesis.pdf
3. <a id="3">[sbowiki]</a> *Sokoban Wiki*, http://sokobano.de/wiki/index.php?title=Main_Page
4. <a id="4">[sboYass]</a> *Brian Damgaard about the YASS solver*, http://sokobano.de/wiki/index.php?title=Sokoban_solver_%22scribbles%22_by_Brian_Damgaard_about_the_YASS_solver
5. <a id="5">[pdb]</a> Optimal Sokoban solving using pattern databases with specific domain knowledge, https://www.sciencedirect.com/science/article/pii/S0004370215000867#br0030
6. <a id="6">[AL]</a> Hungarian impl. by Andrey Lopatin, http://e-maxx.ru/algo/assignment_hungary
