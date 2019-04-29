#include <utility>
#include <atomic>
#include <tuple>
#include <time.h>
#include <iostream>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

struct Succ;
struct Node;

pthread_mutex_t lock;

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
			union {
				struct {
  		  	int flag;
			    int mark;
				};
				size_t together;
			};
    } __attribute__ (( __aligned__( 16 ) ));
	};
	public:
		Succ(Node* r) {
			right = r;
			flag = 0;
			mark = 0;
		}
		Succ(Node* r, int m, int f) {
			right = r;
			flag = f;
			mark = m;
		}

		bool cas(Succ const& newSucc, Succ const& expectedSucc) {
      bool result;
      __asm__ __volatile__ (
          "lock cmpxchg16b %1\n\t"
          "setz %0\n"
          : "=q" ( result )
           ,"+m" ( ui )
          : "a" ( expectedSucc.right ), "d" ( expectedSucc.together )
           ,"b" ( newSucc.right ), "c" ( newSucc.together )
          : "cc"
      );
      return result;
    }
		bool operator==(const Succ& other) const {
	  	return right == other.right &&
	  				 together == other.together;
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
			curr -> succ = new Succ(new Node(INT_MAX));
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
    assert(prev_node != nullptr);
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
  		nodePair= InsertNode(newNode, prev_node, next_node);
  		prev_node = nodePair.first;
  		Node* result = nodePair.second;
  		if((result == nullptr) && (curr_v == 1)) {
  			delete newNode;
  			return nullptr;
  		}
      assert(newRNode != nullptr);
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

  void printSLFlagMark() {
    Node* curr = head;
    while(curr -> up != curr) {
      curr = curr -> up;
    }
    int maxLvl = maxLevel - 1;
    while(curr != nullptr) {
      std::cout << maxLvl << ": ";
      Node* goRight = curr;
      while(goRight != nullptr) {
        std::cout << "[" << goRight -> key << ", " << goRight -> succ -> mark << ", ";
        std::cout << goRight -> succ -> flag << "] ";
        goRight = goRight -> succ -> right;
      }
      curr = curr -> down;
      maxLvl--;
      std::cout << std::endl;
    }
    std::cout << std::endl;
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
      assert(curr_node != nullptr);
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
    assert(curr_node != nullptr);
  	Node* next_node = curr_node -> succ -> right; 
    if(next_node == nullptr) {
      pthread_mutex_lock(&lock);
      printSLFine();
      std::cout << "\n" << sub(curr_node) << " " << curr_node -> key << "\n\n";
      exit(1);
    }
    assert(next_node != nullptr); 		  	
  	while(next_node -> key <= k) {

      assert(next_node != nullptr);   
  		while(next_node -> key != INT_MAX && next_node -> tower_root -> succ -> mark) {
  		  std::tuple<Node*, bool, bool> tup = TryFlagNode(curr_node, next_node);
  		  curr_node = std::get<0>(tup);
  		  if(std::get<1>(tup)) {
  		  	HelpFlagged(curr_node, next_node);
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

  // The second element will true if it has not been logically deleted
  std::tuple<Node*, bool, bool> TryFlagNode(Node* prev_node, Node* target_node) {
  	Succ t01(target_node, 0, 1);
  	Succ t00(target_node, 0, 0);
  	while(true) {
  		if(*(prev_node -> succ) == t01) {
  			return std::make_tuple(prev_node, true, false);
  		}
  		bool success = prev_node -> succ -> cas(t01, t00);
  		if(success) {
  			return std::make_tuple(prev_node, true, true);
  		}
  		if(*(prev_node -> succ) == t01) {
  			return std::make_tuple(prev_node, true, false);
  		}
  		while(prev_node -> succ -> mark) {
  			prev_node = prev_node -> back_link;
  		}
  		std::pair<Node*, Node*> nodePair = SearchRight(target_node -> key - 1, prev_node);
  		prev_node = nodePair.first;
  		Node* del_node = nodePair.second;
  		if(del_node != target_node) {
  			return std::make_tuple(prev_node, false, false);
  		}
  	}
  	exit(1);
  	return std::make_tuple(nullptr, false, false);
  }


  std::pair<Node*, Node*> InsertNode(Node* newNode, Node* prev_node, Node* next_node) {
    assert(prev_node != nullptr);
    assert(newNode != nullptr);
  	if(prev_node -> key == newNode -> key) {
  		return std::make_pair(prev_node, nullptr);
  	}
  	while(true) {
      assert(prev_node != nullptr);
  		Succ* prev_succ = prev_node -> succ;
  		if(prev_succ -> flag) {
  			HelpFlagged(prev_node, prev_succ -> right);
  		}
  		else {
        assert(newNode != nullptr);
  			newNode -> succ = new Succ(next_node, 0, 0);
  			Succ expectedSucc(next_node, 0, 0);
  			Succ newSucc(newNode, 0, 0);
        assert(prev_node != nullptr);
  			bool success = prev_node -> succ -> cas(newSucc, expectedSucc);
  			if(success) {
  				return std::make_pair(prev_node, newNode);
  			}
  			else {
          assert(prev_node != nullptr);
  				if(!(prev_node -> succ -> flag) && prev_node -> succ -> mark) {
  					HelpFlagged(prev_node, expectedSucc.right);
  				}

          assert(prev_node != nullptr);
  				while(prev_node -> succ -> mark) {
  					prev_node = prev_node -> back_link;
            assert(prev_node != nullptr);
  				}
  			}
  		}
      assert(newNode != nullptr);
      std::pair<Node*, Node*> nodePair = SearchRight(newNode -> key, prev_node);
      prev_node = nodePair.first;
      next_node = nodePair.second;

      assert(prev_node != nullptr);

      assert(next_node != nullptr);
  		if(prev_node -> key == next_node -> key) {
  			return std::pair<Node*, Node*>(prev_node, nullptr);
  		}
  	}
  	exit(1);
  	return std::pair<Node*, Node*>(nullptr, nullptr);
  }


  Node* DeleteNode(Node* prev_node, Node* del_node) {
  	std::tuple<Node*, bool, bool> tup = TryFlagNode(prev_node, del_node);
  	prev_node = std::get<0>(tup);
  	bool status = std::get<1>(tup);
  	bool result = std::get<2>(tup);
  	if(status) {
  		HelpFlagged(prev_node, del_node);
  	}
  	if(!result) {
  		return nullptr;
  	}
  	return del_node;
  }

  void HelpMarked(Node* prev_node, Node* del_node) {
  	Node* next_node = del_node -> succ -> right;
  	Succ expectedSucc(del_node, 0, 1);
  	Succ newSucc(next_node, 0, 0);
  	prev_node -> succ -> cas(newSucc, expectedSucc);
  }

  void HelpFlagged(Node* prev_node, Node* del_node) {
  	del_node -> back_link = prev_node;
  	if(!(del_node -> succ -> mark)) {
  		TryMark(del_node);
  	}
  	HelpMarked(prev_node, del_node);
  }

  void TryMark(Node* del_node) {
  	do {
  		Node* next_node = del_node -> succ -> right;
  		Succ expectedSucc(next_node, 0, 0);
  		Succ newSucc(next_node, 1, 0);
  		bool success = del_node -> succ -> cas(newSucc, expectedSucc);
  		if(!success && !(del_node -> succ -> flag) && del_node -> succ -> mark) {
  			HelpFlagged(del_node, del_node -> succ -> right);
  		}
  	} while(!(del_node -> succ -> mark));
  }
};

void* producer(void* ptr) {
  SkipList sl = *(SkipList*) ptr;
  for(int i = 0; i < 300; i++) {
  	sl.Insert_SL(i);
  	if(i % 10 == 0) {
  		usleep(5);
  	}
  }
  return nullptr;
}

void* consumer(void* ptr) {

  SkipList sl = *(SkipList*) ptr;
  for(int i = 0; i < 300; i++) {
  	sl.Delete_SL(i);
  	if(i % 10 == 0) {
  		usleep(5);
  	}
  }
  return nullptr;
}

void* inspector(void* ptr) {
  SkipList sl = *(SkipList*) ptr;
  for(int i = 0; i < 300; i++) {
  	sl.Search_SL(i);
  	if(i % 10 == 0) {
  		usleep(5);
  	}
  }
  return nullptr;
}

void spawn1() {
  SkipList* sl = new SkipList(10);

  pthread_t pthreads[3];
  pthread_create(pthreads, NULL, producer, (void*) sl);
  pthread_create(pthreads + 1, NULL, producer, (void*) sl);
  pthread_create(pthreads + 2, NULL, producer, (void*) sl);
  int i = pthread_join(pthreads[0], NULL);
  i += pthread_join(pthreads[1], NULL);
  i += pthread_join(pthreads[2], NULL);
  /*for(int i = 0; i < 300; i++) {
    Node* node = sl -> Search_SL(i);
    assert(node -> key == i);
  }*/
  //sl -> printSLRough();
}

void spawn2() {
  SkipList* sl = new SkipList(10);
  pthread_t pthreads[3];
  pthread_create(pthreads, NULL, producer, (void*) sl);
  pthread_create(pthreads + 1, NULL, consumer, (void*) sl);
  pthread_create(pthreads + 2, NULL, inspector, (void*) sl);
  pthread_join(pthreads[0], NULL);
  pthread_join(pthreads[1], NULL);
  pthread_join(pthreads[2], NULL);
  sl -> printSLRough();
}

void* tester(void* ptr) {
  SkipList sl = *(SkipList*) ptr;
  for(int i = 0; i < 300; i++) {
    sl.Insert_SL(i % 30);
    if(i % 10 == 0) {
      usleep(5);
    }
  }
  return nullptr;
}

void spawn3() {
  SkipList* sl = new SkipList(10);

  pthread_t pthreads[3];
  pthread_create(pthreads, NULL, tester, (void*) sl);
  pthread_create(pthreads + 1, NULL, tester, (void*) sl);
  pthread_create(pthreads + 2, NULL, tester, (void*) sl);
  int i = pthread_join(pthreads[0], NULL);
  i += pthread_join(pthreads[1], NULL);
  i += pthread_join(pthreads[2], NULL);
}


int main() {
  spawn3();
  /*
  SkipList sl(5);
  sl.Insert_SL(10);
  sl.Insert_SL(20);
  sl.Insert_SL(30);
  sl.printSLFine();
  */
}