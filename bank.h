#ifndef _BANK_H
#define _BANK_H

typedef int64_t AccountAmount;

typedef struct Bank {
  unsigned int numberBranches;
  struct       Branch  *branches;
  struct       Report  *report;

  pthread_mutex_t lock;
  AccountAmount balance;

  int numWorking;

  pthread_mutex_t doLock;
  pthread_cond_t reportCond;
} Bank;

#include "account.h"

int Bank_Balance(Bank *bank, AccountAmount *balance);

Bank *Bank_Init(int numBranches, int numAccounts, AccountAmount initAmount,
                AccountAmount reportingAmount,
                int numWorkers);

int Bank_Validate(Bank *bank);
int Bank_Compare(Bank *bank1, Bank *bank2);

int Bank_UpdateBalance(Bank *bank, AccountAmount change);


#endif /* _BANK_H */
 