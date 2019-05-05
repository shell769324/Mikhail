import numpy as np 
import sys
# 0 is Search
# 1 is Insert
# 2 is Delete

'''
1 34
1 45
1 23
2 34 
'''

def create_file(filename,max_lines):
    
    insert_set = []
    final = []

    insert_pc = 1.0
    search_pc = 0.0
    delete_pc = 0.0

    correct_delete_pc = 0.5
    correct_search_pc = 0.5
    no_insert = 0
    no_delete = 0
    no_search = 0
    int_max = 2147483647 - 5
    int_min = -2147483648 +5
    #int_max = 10
    #int_min =0

    # first insert -inf and +inf
    final.append(str(1) + ' ' + str(int_max))
    final.append(str(2) + ' ' + str(int_min))
    for i in range(max_lines):
        if i%(max_lines//5) ==0:
            print(i)
        op = np.random.choice(3, 1, p=[search_pc, insert_pc, delete_pc])
        #print("op = ",op)
        if i==0:
            op=1
        #search
        if op == 0:
            if(len(insert_set) ==0):
                continue
            search_for1 =  np.random.randint(int_min,int_max)
            search_for2 = np.random.randint(0,len(insert_set))
            u = np.random.choice(2,1,p=[1 -correct_search_pc , correct_search_pc])
            if u==0:
                search_for = search_for1 
            else:
                search_for = insert_set[search_for2] # string op here

            word =  str(0) +' '+ str(search_for)
            no_search += 1
            #print("searching ",search_for)

        #insert
        elif op ==1:
            random_insert = np.random.randint(int_min,int_max)
            word = str(1) + ' ' + str(random_insert)
            no_insert+=1
            '''if not  (random_insert in insert_set):
                insert_set.append(random_insert)'''
            #print("inserting ",random_insert)

        elif op ==2:
            if(len(insert_set) ==0):
                continue
            delete_this1 =  np.random.randint(int_min,int_max)
            delete_this2 = np.random.randint(0,len(insert_set))
            u = np.random.choice(2,1,p=[1 - correct_delete_pc,correct_delete_pc])
            if u==0:
                delete_this = delete_this1 
            else:
                delete_this = insert_set[delete_this2] # string op here
                del insert_set[delete_this2]

            word =  str(2) +' '+ str(delete_this)
            no_delete += 1
            #print("deleting ",delete_this)
        #print("insert_set = ",insert_set)
        final.append(word)

    print(len(final))

    f = open("./files/random_"+str(max_lines)+".txt","w+")
    for word in final:
        f.write(word+'\n')
    f.close()

def split_file(lineList,num_threads,max_lines):
    l = len(lineList)   
    k=0 
    for i in range(0,l,l//num_threads):
        list_file= lineList[i:i+l//num_threads]
        f = open("./files/"+filename+"_"+str(max_lines)+"_"+str(k)+".txt","w+")
        for line in list_file:
            f.write(line+'\n')
        f.close()
        k+=1

def split_file_delete(lineList,num_threads,max_lines):
    l = len(lineList)   
    k=0 
    for i in range(0,l,l//num_threads):
        list_file= lineList[i:i+l//num_threads]
        f = open("./files/"+filename+"delete_"+str(max_lines)+"_"+str(k)+".txt","w+")
        for line in list_file:
            f.write(line+'\n')
        f.close()
        k+=1
def split_file_search(lineList,num_threads,max_lines):
    l = len(lineList)   
    k=0 
    for i in range(0,l,l//num_threads):
        list_file= lineList[i:i+l//num_threads]
        f = open("./files/"+filename+"search_"+str(max_lines)+"_"+str(k)+".txt","w+")
        for line in list_file:
            f.write(line+'\n')
        f.close()
        k+=1


if __name__ == "__main__":
    filename = "random"
    NUM_THREADS = int(sys.argv[1])
    MAX_LINES = int(sys.argv[2])
    PURE_SEARCH = 1
    PURE_DELETE = 0
    import os
    exists = os.path.isfile("./files/"+filename+"_"+str(MAX_LINES)+".txt")
    if not exists:
        create_file(filename,MAX_LINES)
    lineList = [line.rstrip('\n') for line in open("./files/"+filename+"_"+str(MAX_LINES)+".txt")]
    split_file(lineList,NUM_THREADS,MAX_LINES)
    if PURE_DELETE==1:
        lineList = [line.rstrip('\n') for line in open("./files/"+filename+"delete_"+str(MAX_LINES)+".txt")]
        split_file_delete(lineList,NUM_THREADS,MAX_LINES)
    elif PURE_SEARCH==1:
        lineList = [line.rstrip('\n') for line in open("./files/"+filename+"search_"+str(MAX_LINES)+".txt")]
        split_file_search(lineList,NUM_THREADS,MAX_LINES)
        
    