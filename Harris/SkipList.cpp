#include <utility>
#include <tuple>
#include <time.h>
#include <iostream>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <mutex>

struct Succ;
struct Node;
std::mutex lk;

std::string sub(Node* address) {
  if(address == nullptr) {
    return std::to_string(0);
  }
  std::string s = std::to_string((long) address);
  return s.substr(s.size() - 5, 5);
}

void start(std::string place) {
  std::cout << "start: " << place << "\n";
}

void end(std::string place) {
  std::cout << "end: " << place << "\n";
}


struct Succ {
  union {
    uint64_t ui[2];
    struct {
      Node* right;
      uint64_t mark;
    } __attribute__ (( __aligned__( 16 ) ));
  };
  public:
    Succ(Node* r) {
      right = r;
      mark = 0;
    }
    Succ(Node* r, uint64_t m) {
      right = r;
      mark = m;
    }

    bool cas(Succ const& newSucc, Succ const& expectedSucc) {
      bool result;
      __asm__ __volatile__ (
          "lock cmpxchg16b %1\n\t"
          "setz %0\n"
          : "=q" ( result )
           ,"+m" ( ui )
          : "a" ( expectedSucc.right ), "d" ( expectedSucc.mark )
           ,"b" ( newSucc.right ), "c" ( newSucc.mark )
          : "cc"
      );
      return result;
    }
    bool operator==(const Succ& other) const {
      return right == other.right &&
             mark == other.mark;
    }
};

struct Node {
  Succ* succ;
  int key;
  Node* down;
  union {
    Node* back_link;
    Node* up;
  };
  Node* tower_root;
  public:
    Node(int k) {
      key = k;
      succ = new Succ(nullptr);
    }
    Node(int k, Node* d, Node* towerRoot) {
      key = k;
      down = d;
      tower_root = towerRoot;
      succ = new Succ(nullptr);
    }
};

class SkipList {
public:

  SkipList(int maxLvl) {
    head = new Node(INT_MIN, nullptr, nullptr);
    Node* curr = head;
    maxLevel = maxLvl;
    for(int i = 1; i < maxLevel; i++) {
      Node* next = new Node(INT_MIN, nullptr, nullptr);
      curr -> up = next;
      curr -> succ = new Succ(new Node(INT_MAX, nullptr, nullptr));
      next -> down = curr;
      curr = next;
    }
    curr -> up = curr;
    curr -> succ = new Succ(new Node(INT_MAX));
    seed = 0;
  }

  Node* Search_SL(int k) {
    // (curr_node, next_node)
    std::pair<Node*, Node*> neighbors = SearchToLevel_SL(k, 1);
    Node* curr_node = neighbors.first;
    // curr_node
    if(curr_node -> key == k) {
      return curr_node;
    }
    return nullptr;
  }

  Node* Insert_SL(int k) {
    std::pair<Node*, Node*> nodePair = SearchToLevel_SL(k, 1);
    Node* prev_node = nodePair.first;
    Node* next_node = nodePair.second;
    if(prev_node -> key == k) {
      return nullptr;
    }
    Node* newRNode = new Node(k, nullptr, nullptr);
    newRNode -> tower_root = newRNode;
    Node* newNode = newRNode;
    int tH = 1;
    while(getRand() && (tH <= maxLevel - 2)) {
      tH++;
    }
    int curr_v = 1;
    while(true) {
      nodePair = InsertNode(newNode, prev_node, next_node);
      prev_node = nodePair.first;
      Node* result = nodePair.second;
      if((result == nullptr) && (curr_v == 1)) {
        delete newNode;
        return nullptr;
      }
      if(newRNode -> succ -> mark) {
        if((result == newNode) && (newNode != newRNode)) {
          DeleteNode(prev_node, newNode);
        }
        return newRNode;
      }
      curr_v++;
      if(curr_v == tH + 1) {
        return newRNode;
      }
      Node* lastNode = newNode;
      newNode = new Node(k, lastNode, newRNode);
      nodePair = SearchToLevel_SL(k, curr_v);
      prev_node = nodePair.first;
      next_node = nodePair.second;
    }
    exit(1);
    return nullptr;
  }

  Node* Delete_SL(int k) {
    std::pair<Node*, Node*> nodePair = SearchToLevel_SL(k - 1, 1);
    Node* prev_node = nodePair.first;
    Node* del_node = nodePair.second;
    if(del_node -> key != k) {
      return nullptr;
    }
    Node* result = DeleteNode(prev_node, del_node);
    if(result == nullptr) {
      return nullptr;
    }
    SearchToLevel_SL(k, 2);
    return del_node;
  }


  void printSLRough() {
    lk.lock();
    Node* curr = head;
    while(curr -> up != curr) {
      curr = curr -> up;
    }
    int maxLvl = maxLevel - 1;
    while(curr != nullptr) {
      std::cout << maxLvl << ": ";
      Node* goRight = curr;
      while(goRight != nullptr) {
        std::cout << goRight -> key << " ";
        goRight = goRight -> succ -> right;
      }
      curr = curr -> down;
      maxLvl--;
      std::cout << std::endl;
    }
    std::cout << std::endl;
    lk.unlock();
  }

  void printSLFine() {
    Node* curr = head;
    while(curr -> up != curr) {
      curr = curr -> up;
    }
    int maxLvl = maxLevel - 1;
    while(curr != nullptr) {
      std::cout << maxLvl << ": ";
      Node* goRight = curr;
      while(goRight != nullptr) {
        std::cout << "[" << sub(goRight) << ", " << goRight -> key << ", ";
        std::cout << sub(goRight -> succ -> right) << ", " << sub(goRight -> down) << "] ";
        goRight = goRight -> succ -> right;
      }
      curr = curr -> down;
      maxLvl--;
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }

  void printSLTR() {
    Node* curr = head;
    while(curr -> up != curr) {
      curr = curr -> up;
    }
    int maxLvl = maxLevel - 1;
    while(curr != nullptr) {
      std::cout << maxLvl << ": ";
      Node* goRight = curr;
      while(goRight != nullptr) {
        std::cout << "[" << sub(goRight) << ", " << goRight -> key << ", ";
        std::cout << sub(goRight -> tower_root) << "] ";
        goRight = goRight -> succ -> right;
      }
      curr = curr -> down;
      maxLvl--;
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }

  void printSLMark() {
    lk.lock();
    Node* curr = head;
    while(curr -> up != curr) {
      curr = curr -> up;
    }
    int maxLvl = maxLevel - 1;
    while(curr != nullptr) {
      std::cout << maxLvl << ": ";
      Node* goRight = curr;
      while(goRight != nullptr) {
        std::cout << "[" << sub(goRight) << ", " << goRight -> key << ", ";
        std::cout << goRight -> succ -> mark  << "] ";
        goRight = goRight -> succ -> right;
      }
      curr = curr -> down;
      maxLvl--;
      std::cout << std::endl;
    }
    std::cout << std::endl;
    lk.unlock();
  }

  // No duplicate nodes
  // No superfluous towers
  bool isValid() {
    Node* curr = head;
    while(curr != nullptr) {
      Node* goRight = curr;
      while(goRight != nullptr) {
        if(goRight -> key != INT_MIN && goRight -> key != INT_MAX) {
          if(goRight -> tower_root -> succ -> mark) {
            return false;
          }
        }
        if(goRight -> succ -> right != nullptr) {
          if(goRight -> key >= goRight -> succ -> right -> key) {
            return false;
          }
        }
        goRight = goRight -> succ -> right;
      }
      curr = curr -> down;
    }
    return true;
  }

private:

  Node* head; // the bottom of the head tower
  int maxLevel;
  int seed;

  int getRand() {
    seed = (733 * seed + 181) % 1024;
    return seed >= 512;
  }

  std::pair<Node*, Node*> SearchToLevel_SL(int k, int v) {
    // (curr_node, curr_v)
    std::pair<Node*, int> curr = FindStart_SL(v);
    Node* curr_node = curr.first;
    int curr_v = curr.second;
    while(curr_v > v) {
      // (curr_node, next_node)
      std::pair<Node*, Node*> next = SearchRight(k, curr_node);
      curr_node = next.first;

      curr_node = curr_node -> down;
      curr_v--;
    }
    return SearchRight(k, curr_node);
  }
  
  std::pair<Node*, int> FindStart_SL(int v) {
    Node* curr_node = head;
    int curr_v = 1;
    while(curr_node -> up -> succ -> right -> key != INT_MAX || curr_v < v) {
      curr_node = curr_node -> up;
      curr_v++;
    }
    return std::make_pair(curr_node, curr_v);
  }

  std::pair<Node*, Node*> SearchRight(int k, Node* curr_node) {
    Node* next_node = curr_node -> succ -> right;
    while(next_node -> key <= k) {
      // If the next node is not tail node and the next node's tower root
      // has been deleted, mark next node and attemp to delete it
      while(next_node -> key != INT_MAX && next_node -> tower_root -> succ -> mark) {
        next_node -> back_link = curr_node;
        TryMark(next_node);
        HelpMarked(curr_node, next_node);
        // If the current node is marked, back_linke
        while(curr_node -> succ -> mark) {
          curr_node = curr_node -> back_link;
        }
        next_node = curr_node -> succ -> right;
      }
      if(next_node -> key <= k) {
        curr_node = next_node;
        next_node = curr_node -> succ -> right;
      }
    }
    return std::make_pair(curr_node, next_node);
  }

  std::pair<Node*, Node*> InsertNode(Node* newNode, Node* prev_node, Node* next_node) {
    // If the prev_node has equal key as the newNode,
    // return to notify duplicate key
    if(prev_node -> key == newNode -> key) {
      return std::make_pair(prev_node, nullptr);
    }
    while(true) {
      // Set the successor of the new node to the next node
      newNode -> succ = new Succ(next_node, 0);
      Succ expectedSucc(next_node, 0);
      Succ newSucc(newNode, 0);
      // Attemp to point the prev_node to the new node
      if(prev_node -> succ -> cas(newSucc, expectedSucc)) {
        // If successful, insertion is completed
        return std::make_pair(prev_node, newNode);
      }
      // Otherwise, prev_node may get a new successor or
      // prev_node is being deleted. Use back_link to get to
      // the nearest unmarked node
      while(prev_node -> succ -> mark) {
        prev_node = prev_node -> back_link;
      }
      // Search the key in the chain starting at the new prev_node
      std::pair<Node*, Node*> nodePair = SearchRight(newNode -> key, prev_node);
      // Update the prev_node and next_node
      prev_node = nodePair.first;
      next_node = nodePair.second;
      // If another thread has inserted the key
      if(prev_node -> key == newNode -> key) {
        // return immediately and notify duplicate key
        return std::pair<Node*, Node*>(prev_node, nullptr);
      }
    }
    exit(1);
    return std::pair<Node*, Node*>(nullptr, nullptr);
  }


  Node* DeleteNode(Node* prev_node, Node* del_node) {
    // Before marking, must set the back_link so other threads
    // can go back
    del_node -> back_link = prev_node;
    // Mark del_node before physical deletion
    bool thisCallMark = TryMark(del_node);
    // Physically delete del_node
    HelpMarked(prev_node, del_node);
    // If marking is not done by the TryMark above
    // return nullptr to signify that this node doesn't exist
    if(!thisCallMark) {
      return nullptr;
    }
    // Otherwise, return the del_node
    return del_node;
  }

  void HelpMarked(Node* prev_node, Node* del_node) {
    while(true) {
      // Get the next node of the node to delete
      Node* next_node = del_node -> succ -> right;
      Succ expectedSucc(del_node, 0);
      Succ newSucc(next_node, 0);
      // Try to point prev_node to the next node
      if(prev_node -> succ -> cas(newSucc, expectedSucc)) {
        return;
      }
      // If the prev node is mark, go back to the last node that
      // is not mark
      while(prev_node -> key != INT_MIN && prev_node -> succ -> mark) {
        prev_node = prev_node -> back_link;
      }
      // Search the del_node again
      std::pair<Node*, Node*> nodePair = SearchRight(del_node -> key - 1, prev_node);
      prev_node = nodePair.first;
      Node* new_del_node = nodePair.second;
      // If the same del_node cannot be found, some other thread
      // has deleted. Thus, return.
      if(del_node != new_del_node) {
        return;
      }
    }
  }

  bool TryMark(Node* del_node) {
    do {
      // Get the next node
      Node* next_node = del_node -> succ -> right;
      Succ expectedSucc(next_node, 0);
      Succ newSucc(next_node, 1);
      // Attempt to mark the succ field
      if(del_node -> succ -> cas(newSucc, expectedSucc)) {
        // If marking succeeded, return true to signify that
        // this function call marks the del_node
        return true;
      }
      // If del_node has been marked by some other thread
    } while(!(del_node -> succ -> mark));
    // del_node is logically deleted by an eariler deletion
    // by that thread
    return false;
  }
};

void* producer(void* ptr) {
  SkipList sl = *(SkipList*) ptr;
  for(int i = 0; i < 10000; i++) {
    sl.Insert_SL(i);
    if(i % 10 == 0) {
      usleep(5);
    }
  }
  return nullptr;
}

void* consumer(void* ptr) {
  SkipList sl = *(SkipList*) ptr;
  for(int i = 0; i < 10000; i++) {
    sl.Delete_SL(i);
    if(i % 10 == 0) {
      usleep(5);
    }
  }
  return nullptr;
}

void* inspector(void* ptr) {
  SkipList sl = *(SkipList*) ptr;
  for(int i = 0; i < 100000; i++) {
    sl.Search_SL(i);
    if(i % 10 == 0) {
      usleep(5);
    }
  }
  return nullptr;
}

int main() {
  SkipList* sl = new SkipList(10);
  pthread_t pthreads[15];
  for(int i = 0; i < 15; i++) {
    int modi = i % 3;
    pthread_create(pthreads + i, nullptr,
      modi == 0 ? producer : (modi == 1 ? consumer : inspector), (void*) sl);    
  }
  for(int i = 0; i < 15; i++) {
    pthread_join(pthreads[i], nullptr);
  }
  assert(sl -> isValid());
  std::cout << "success" << "\n";
}