#include <utility>
#include <tuple>
#include <time.h>
#include <iostream>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <iomanip>
#include <random>
#include <map>

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

bool isMarkedRef(Node* node) {
  return ((long) node & 0x1L) == 0x1L;
}

bool isFlaggedRef(Node* node) {
  return ((long) node & 0x2L) == 0x2L;
}

Node* markRef(Node* node) {
  return (Node*) ((long) node | 0x1L);
}

Node* flagRef(Node* node) {
  return (Node*) ((long) node | 0x2L);
}

Node* unBothRef(Node* node) {
  return (Node*) ((long) node & ~0x3L);
}


struct Node {
	Node* right;
	Node* down;
	union {
		Node* back_link;
		Node* up;
	};
	Node* tower_root;
  int key;
  public:
  	Node(int k) {
  		key = k;
      right = nullptr;
      down = nullptr;
      tower_root = nullptr;
  	}
  	Node(int k, Node* d, Node* towerRoot) {
  		key = k;
  		down = d;
  		tower_root = towerRoot;
      right = nullptr;
  	}
};

class SkipList {
public:

	SkipList(int maxLvl) {
		head = new Node(INT_MIN);
		Node* curr = head;
		maxLevel = maxLvl;

    std::random_device rd;
    gen = std::mt19937(rd());
    d = std::bernoulli_distribution(0.5);
		for(int i = 1; i < maxLevel; i++) {
			Node* next = new Node(INT_MIN);
			curr -> up = next;
			curr -> right = new Node(INT_MAX);
			next -> down = curr;
			curr = next;
		}
		curr -> up = curr;
		curr -> right = new Node(INT_MAX);
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
  	Node* newRNode = new Node(k);
  	newRNode -> tower_root = newRNode;
  	Node* newNode = newRNode;
  	int tH = 1;
  	while(d(gen) && (tH <= maxLevel - 2)) {
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
  		if(isMarkedRef(newRNode -> right)) {
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

private:

	Node* head; // the bottom of the head tower
	int maxLevel;
	int seed;
  std::mt19937 gen;
  std::bernoulli_distribution d;

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

  	while(unBothRef(curr_node -> up -> right) -> key != INT_MAX || curr_v < v) {
  		curr_node = curr_node -> up;
  		curr_v++;
  	}

  	return std::make_pair(curr_node, curr_v);
  }

  std::pair<Node*, Node*> SearchRight(int k, Node* curr_node) {
  	Node* next_node = unBothRef(curr_node -> right);
  	while(next_node -> key <= k) {
  		while(next_node -> key != INT_MAX && isMarkedRef(next_node -> tower_root -> right)) {
  		  std::tuple<Node*, bool, bool> tup = TryFlagNode(curr_node, next_node);
  		  curr_node = std::get<0>(tup);
  		  if(std::get<1>(tup)) {
          HelpFlagged(curr_node, next_node);
  		  }
  		  next_node = unBothRef(curr_node -> right);
  		}

  		if(next_node -> key <= k) {
  			curr_node = next_node;
  			next_node = unBothRef(curr_node -> right);
  		}
  	}
  	return std::make_pair(curr_node, next_node);
  }

  // The second element will true if it has not been logically deleted
  std::tuple<Node*, bool, bool> TryFlagNode(Node* prev_node, Node* target_node) {
    Node* flagged_target_node = flagRef(target_node);
  	while(true) {
  		if(isFlaggedRef(prev_node -> right)) {
  			return std::make_tuple(prev_node, true, false);
  		}
  		if(__sync_bool_compare_and_swap(&(prev_node -> right),
          target_node, flagged_target_node)) {
  			return std::make_tuple(prev_node, true, true);
  		}
  		if(isFlaggedRef(prev_node -> right)) {
  			return std::make_tuple(prev_node, true, false);
  		}
  		while(isMarkedRef(prev_node -> right)) {
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
    
  	if(prev_node -> key == newNode -> key) {
  		return std::make_pair(prev_node, nullptr);
  	}
  	while(true) {
  		Node* right = prev_node -> right;
  		if(isFlaggedRef(right)) {
  			HelpFlagged(prev_node, unBothRef(right));
  		}
  		else {
  			newNode -> right = next_node;
  			if(__sync_bool_compare_and_swap(
          &(prev_node -> right), next_node, newNode)) {
  				return std::make_pair(prev_node, newNode);
  			}
        Node* curr_right = prev_node -> right;
  			if(isFlaggedRef(curr_right) && !isMarkedRef(curr_right)) {
  				HelpFlagged(prev_node, next_node);
  			}
  			while(isMarkedRef(prev_node -> right)) {
  				prev_node = prev_node -> back_link;
  			}
  		}
      std::pair<Node*, Node*> nodePair = SearchRight(newNode -> key, prev_node);
      prev_node = nodePair.first;
      next_node = nodePair.second;
  		if(prev_node -> key == newNode -> key) {
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
  	Node* next_node = unBothRef(del_node -> right);
    Node* flagged_del_node = flagRef(del_node);
  	__sync_bool_compare_and_swap(&(prev_node -> right),
      flagged_del_node, next_node);
  }

  void HelpFlagged(Node* prev_node, Node* del_node) {
  	del_node -> back_link = prev_node;
  	if(!isMarkedRef(del_node -> right)) {
  		TryMark(del_node);
  	}
  	HelpMarked(prev_node, del_node);
  }

  void TryMark(Node* del_node) {
    if(del_node -> key == INT_MAX) {
      return;
    }
  	do {
  		Node* next_node = unBothRef(del_node -> right);
      Node* marked_next_node = markRef(next_node);
  		bool success = __sync_bool_compare_and_swap(
       &(del_node -> right), next_node, marked_next_node);
      Node* del_right = del_node -> right;
  		if(!success && !isMarkedRef(del_right) && isFlaggedRef(del_right)) {
  			HelpFlagged(del_node, unBothRef(del_right));
  		}
  	} while(!isMarkedRef(del_node -> right));
  }
};

void* producer(void* ptr) {
  SkipList sl = *(SkipList*) ptr;
  for(int i = 0; i < 100000; i++) {
  	sl.Insert_SL(i);
  	/*if(i % 10 == 0) {
  		usleep(5);
  	}*/
  }
  return nullptr;
}

void* consumer(void* ptr) {
  SkipList sl = *(SkipList*) ptr;
  for(int i = 0; i < 100000; i++) {
  	sl.Delete_SL(i);
  	/*if(i % 10 == 0) {
  		usleep(5);
  	}*/
  }
  return nullptr;
}

void* inspector(void* ptr) {
  SkipList sl = *(SkipList*) ptr;
  for(int i = 0; i < 100000; i++) {
  	sl.Search_SL(i);/*
  	if(i % 10 == 0) {
  		usleep(5);
  	}*/
  }
  return nullptr;
}