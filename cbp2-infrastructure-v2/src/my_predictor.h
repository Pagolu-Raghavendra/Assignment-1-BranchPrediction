// my_predictor.h
// This file contains a sample gshare_predictor class.
// It is a simple 32,768-entry gshare with a history length of 15.

class gshare_update : public branch_update {
public:
	unsigned int index;
};

class gshare_predictor : public branch_predictor {
public:
#define HISTORY_LENGTH	15
#define TABLE_BITS	15
	gshare_update u;
	branch_info bi;
	unsigned int history;
	unsigned char tab[1<<TABLE_BITS];

	gshare_predictor (void) : history(0) { 
		memset (tab, 0, sizeof (tab));
	}

	branch_update *predict (branch_info & b) {
		bi = b;
		if (b.br_flags & BR_CONDITIONAL) {
			u.index = 
				  (history << (TABLE_BITS - HISTORY_LENGTH)) 
				^ (b.address & ((1<<TABLE_BITS)-1));
			u.direction_prediction (tab[u.index] >> 1);
		} else {
			u.direction_prediction (true);
		}
		u.target_prediction (0);
		return &u;
	}

	void update (branch_update *u, bool taken, unsigned int target) {
		if (bi.br_flags & BR_CONDITIONAL) {
			unsigned char *c = &tab[((gshare_update*)u)->index];
			if (taken) {
				if (*c < 3) (*c)++;
			} else {
				if (*c > 0) (*c)--;
			}
			history <<= 1;
			history |= taken;
			history &= (1<<HISTORY_LENGTH)-1;
		}
	}
};
#ifndef PM_Bimodal_Bits
#define PM_Bimodal_Bits  12
#endif
#ifndef PM_History_Bits
#define PM_History_Bits  15
#endif
#ifndef PM_Tag_Bits
#define PM_Tag_Bits        6
#endif
#ifndef PM_Set_Bits
#define PM_Set_Bits        9
#endif
#ifndef PM_Ways
#define PM_Ways            4
#endif
#ifndef PM_Mispredict
#define PM_Mispredict 1
#endif
#ifndef PM_Bimodal_Only
#define PM_Bimodal_Only    0
#endif
//
// Pentium M hybrid branch predictors
// This class implements a simple hybrid branch predictor based on the Pentium M branch outcome prediction units. 
// Instead of implementing the complete Pentium M branch outcome predictors, the class below implements a hybrid 
// predictor that combines a bimodal predictor and a global predictor. 
class pm_update : public branch_update {
public:
        unsigned int index;
		unsigned int tag;
		unsigned int set;
		int hitway;
		bool bimodalprediction;
};
struct pm_entry {
		bool valid;
		unsigned char tag;
		unsigned char counter;
		unsigned char lru;
};

class pm_predictor : public branch_predictor {
public:
        pm_update u;
		branch_info bi;
		unsigned int history;
		unsigned char bimodal[1<<PM_Bimodal_Bits];
		pm_entry global[1<<PM_Set_Bits][PM_Ways];

        pm_predictor (void): history(0) {
			memset (bimodal,1,sizeof(bimodal));
			for (int i=0;i<(1<<PM_Set_Bits);i++){
				for (int j=0;j<PM_Ways;j++){
					global[i][j].valid=false;
					global[i][j].tag=0;
					global[i][j].counter=0;
					global[i][j].lru=j;
				}
			}
        }
		static void train_counter(unsigned char &c,bool taken){
			if(taken){
				if(c<3) c++;
			} else {
				if(c>0) c--;
			}
		}
		void touch_lru(unsigned int set,unsigned char way){
			unsigned char oldlru=global[set][way].lru;
			for(int i=0;i<PM_Ways;i++){
				if(global[set][i].lru<oldlru){
					global[set][i].lru++;
				}
			}
			global[set][way].lru=0;
		}
		int victim_lru(unsigned int set){
			int lruway=0;
			for(int i=0;i<PM_Ways;i++){
				if(!global[set][i].valid){
					return i;
				}
				if(global[set][i].lru>global[set][lruway].lru){
					lruway=i;
				}
			}
			return lruway;
		}
        branch_update *predict (branch_info & b) {
			bi=b;
            if (b.br_flags&BR_CONDITIONAL){
				u.index=b.address&((1<<PM_Bimodal_Bits)-1);
                u.bimodalprediction=bimodal[u.index]>>1;
                unsigned int hash=(b.address^history)&((1<<PM_History_Bits)-1);                        u.tag=hash&((1<<PM_Tag_Bits)-1);
                u.set=(hash>>PM_Tag_Bits)&((1<<PM_Set_Bits)-1);
                u.hitway=-1;
                if(!PM_Bimodal_Only){
                    for (int w=0;w<PM_Ways;w++){
                            if (global[u.set][w].valid && global[u.set][w].tag==u.tag){
								u.hitway = w;
                                break;
                            }
					}
				}
                if(u.hitway>=0){
                    u.direction_prediction(global[u.set][u.hitway].counter>>1);
				}
                else{
                    u.direction_prediction(u.bimodalprediction);
                }
			} 
			else {
            	u.direction_prediction(true);
            }
            u.target_prediction(0);
            return &u;
        }

        void update (branch_update *u, bool taken, unsigned int target) {
            if (!(bi.br_flags & BR_CONDITIONAL)){ 
				return;
			}
            pm_update *pu = (pm_update *) u;
            train_counter(bimodal[pu->index],taken);
            if (!PM_Bimodal_Only) {
                if (pu->hitway >= 0) {
                    train_counter(global[pu->set][pu->hitway].counter, taken);
                    touch_lru(pu->set, pu->hitway);
                } 
				else if (!PM_Mispredict || pu->bimodalprediction != taken) {
                    int w = victim_lru(pu->set);
                    global[pu->set][w].valid = true;
                    global[pu->set][w].tag = pu->tag;
                    global[pu->set][w].counter = taken ? 2 : 1;
                    touch_lru(pu->set, w);
                }
            }
            history=((history<<1)|taken)&((1<<PM_History_Bits)-1);
        }

};

#define LPB_Set_Bits 6
#define LPB_Ways     2
#define LPB_Max      63
#define BTB_Set_Bits 9
#define BTB_Ways     4
#define IBTB_Bits    8
#define RAS_Size     16
#ifndef LPB_Conf_TH
#define LPB_Conf_TH  3      
#endif
#ifndef LPB_Min_Limit
#define LPB_Min_Limit 2   
#endif
#ifndef LPB_Over_Global
#define LPB_Over_Global 1
#endif
//
// Complete Pentium M branch predictors for extra credit
// This class implements the complete Pentium M branch prediction units. 
// It implements both branch target prediction and branch outcome predicton. 
class cpm_update : public pm_update {
public:
        unsigned int index;
		unsigned int btb_tag;
		unsigned int btb_set;
		unsigned int lpb_tag;
		unsigned int lpb_set;
		bool lpb_used;
		int lpb_way;
		int btb_way;
		bool pmprediction;
};
struct lpb_entry {
    bool valid, dir;
    unsigned char tag, count, limit, conf, lru;
};

struct btb_entry {
    bool valid;
    unsigned int tag, target, len;
    unsigned char lru;
};
class cpm_predictor : public pm_predictor {
public:
        cpm_update u;
		lpb_entry lpb[1 << LPB_Set_Bits][LPB_Ways];
        btb_entry btb[1 << BTB_Set_Bits][BTB_Ways];
        unsigned int ibtb[1 << IBTB_Bits];
        unsigned int ras[RAS_Size];
        int ras_top;
        cpm_predictor (void):ras_top(0) {
			memset (lpb, 0, sizeof(lpb));
			memset (btb, 0, sizeof(btb));
			memset (ibtb, 0, sizeof(ibtb));
			memset (ras, 0, sizeof(ras));
			for (int i=0;i<(1<<LPB_Set_Bits);i++){
				for (int j=0;j<LPB_Ways;j++){
					lpb[i][j].lru=j;
				}
			}
			for (int i=0;i<(1<<BTB_Set_Bits);i++){
				for (int j=0;j<BTB_Ways;j++){
					btb[i][j].lru=j;
				}
			}
        }
		template <class E, int N> static void lru_touch (E *set, int w) {
                unsigned char old = set[w].lru;
                for (int i = 0; i < N; i++) if (set[i].lru < old) set[i].lru++;
                set[w].lru = 0;
        }
        template <class E, int N> static int lru_victim (E *set) {
                int v = 0;
                for (int i = 0; i < N; i++) {
                        if (!set[i].valid) return i;
                        if (set[i].lru > set[v].lru) v = i;
                }
                return v;
        }
        branch_update *predict (branch_info & b) {
            pm_update *pu = (pm_update *) pm_predictor::predict (b);
            *(pm_update *) &u = *pu;
                u.lpb_used = false;
                u.lpb_way = -1;
                u.pmprediction = pu->direction_prediction ();
                if (b.br_flags & BR_CONDITIONAL) {
                        u.lpb_set = (b.address >> 4) & ((1 << LPB_Set_Bits) - 1);
                        u.lpb_tag = (b.address >> 10) & 0x3f;
                        for (int w = 0; w < LPB_Ways; w++) {
                                lpb_entry &e = lpb[u.lpb_set][w];
                                if (e.valid && e.tag == u.lpb_tag) { u.lpb_way = w; break; }
                        }
                        if (u.lpb_way >= 0 && (u.hitway < 0 || LPB_Over_Global)) {
                                lpb_entry &e = lpb[u.lpb_set][u.lpb_way];
                                if (e.conf >= LPB_Conf_TH && e.limit >= LPB_Min_Limit) {
                                        u.lpb_used = true;
                                        u.direction_prediction (e.count == e.limit ? !e.dir : e.dir);
                                }
                        }
                }

                // ---- target prediction ----
                unsigned int tgt = 0;
                u.btb_set = (b.address >> 4) & ((1 << BTB_Set_Bits) - 1);
                u.btb_tag = (b.address >> 13) & 0x1ff;
                u.btb_way = -1;
                for (int w = 0; w < BTB_Ways; w++)
                        if (btb[u.btb_set][w].valid && btb[u.btb_set][w].tag == u.btb_tag) { u.btb_way = w; break; }
                u.index = (b.address^history) & ((1 << IBTB_Bits) - 1);
                if (b.br_flags & BR_RETURN)
                        tgt = ras[(ras_top + RAS_Size - 1) % RAS_Size];
                else if (b.br_flags & BR_INDIRECT)
                        tgt = ibtb[u.index];
                else if (u.btb_way >= 0) {
                        btb_entry &e = btb[u.btb_set][u.btb_way];
                        bool dir_taken = !(b.br_flags & BR_CONDITIONAL) || u.direction_prediction ();
                        tgt = dir_taken ? e.target : b.address + e.len;
                } else if ((b.br_flags & BR_CONDITIONAL) && !u.direction_prediction ())
                        tgt = b.address + 2;
                u.target_prediction (tgt);
                return &u;
        }

        void update (branch_update *u, bool taken, unsigned int target) {
			cpm_update *c = (cpm_update *) u;
            branch_info b = bi;

                if (b.br_flags & BR_CONDITIONAL) {
                        bool final_wrong = c->direction_prediction () != taken;
                        if (c->lpb_way >= 0) {
                                lpb_entry &e = lpb[c->lpb_set][c->lpb_way];
                                if (taken == e.dir) {
                                        if (e.count < LPB_Max) e.count++;
                                        else { e.valid = false; }      
                                } else {
                                        if (e.count == e.limit) { if (e.conf < 3) e.conf++; }
                                        else { e.limit = e.count; e.conf = 0; }
                                        e.count = 0;
                                }
                                lru_touch<lpb_entry, LPB_Ways> (lpb[c->lpb_set], c->lpb_way);
                        } else if (final_wrong) {
                                int w = lru_victim<lpb_entry, LPB_Ways> (lpb[c->lpb_set]);
                                lpb_entry &e = lpb[c->lpb_set][w];
                                e.valid = true; e.tag = c->lpb_tag; e.dir = !taken;
                                e.count = 0; e.limit = 0; e.conf = 0;
                                lru_touch<lpb_entry, LPB_Ways> (lpb[c->lpb_set], w);
                        }
                        pm_predictor::update (u, taken, target);
                }

                // ---- target structures ----
                if (b.br_flags & BR_CALL) {
                        ras[ras_top] = b.address + 5;   // approximate return address
                        ras_top = (ras_top + 1) % RAS_Size;
                }
                if (b.br_flags & BR_RETURN) {
                        ras_top = (ras_top + RAS_Size - 1) % RAS_Size;
                } else if (b.br_flags & BR_INDIRECT) {
                        ibtb[c->index] = target;
                } else {
                        int w = c->btb_way;
                        if (w < 0) {
                                w = lru_victim<btb_entry, BTB_Ways> (btb[c->btb_set]);
                                btb[c->btb_set][w].valid = true;
                                btb[c->btb_set][w].tag = c->btb_tag;
                                btb[c->btb_set][w].target = target;
                                btb[c->btb_set][w].len = 2;
                        }
                        if (taken) btb[c->btb_set][w].target = target;
                        else       btb[c->btb_set][w].len = target - b.address;
                        lru_touch<btb_entry, BTB_Ways> (btb[c->btb_set], w);
                }
        }

};