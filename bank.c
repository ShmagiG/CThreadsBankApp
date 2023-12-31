#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>


#include "error.h"

#include "bank.h"
#include "branch.h"
#include "account.h"
#include "report.h"

#include <pthread.h>
#include "debug.h"

/*
 * allocate the bank structure and initialize the branches.
 */
Bank*
Bank_Init(int numBranches, int numAccounts, AccountAmount initalAmount,
          AccountAmount reportingAmount,
          int numWorkers)
{

  Bank *bank = malloc(sizeof(Bank));

  if (bank == NULL) {
    return bank;
  }

  bank->balance = 0;

  pthread_mutex_init(&bank->lock, NULL);

  Branch_Init(bank, numBranches, numAccounts, initalAmount);
  Report_Init(bank, reportingAmount, numWorkers);

  bank->numWorking = numWorkers;

  pthread_mutex_init(&bank->doLock, NULL);
  pthread_cond_init(&bank->reportCond, NULL);
  return bank;
}

/*
 * get the balance of the entire bank by adding up all the balances in
 * each branch.
 */
int Bank_UpdateBalance(Bank *bank, AccountAmount change){
  pthread_mutex_lock(&bank->lock);
  AccountAmount oldBalance = bank->balance; Y;
  bank->balance = oldBalance + change; Y;
  pthread_mutex_unlock(&bank->lock);

  return 0;
}
int
Bank_Balance(Bank *bank, AccountAmount *balance)
{
  assert(bank->branches);

  // pthread_mutex_lock(&bank->lock);
  for(unsigned int i = 0; i < bank->numberBranches; i++){
    pthread_mutex_lock(&bank->branches[i].lock);
  }
  AccountAmount bankTotal = 0;
  for (unsigned int branch = 0; branch < bank->numberBranches; branch++) {
    AccountAmount branchBalance;
    int err = bank->branches[branch].balance;
    if (err < 0) {
      for(unsigned int i = 0; i < bank->numberBranches; i++){
        pthread_mutex_unlock(&bank->branches[i].lock);
      }
      return err;
    }
    bankTotal += branchBalance;
  }

  *balance = bankTotal;

  for(unsigned int i = 0; i < bank->numberBranches; i++){
    pthread_mutex_unlock(&bank->branches[i].lock);
  }
  // pthread_mutex_unlock(&bank->lock);
  return 0;
}

/*
 * tranverse and validate each branch.
 */
int
Bank_Validate(Bank *bank)
{
  assert(bank->branches);
  int err = 0;
  for(unsigned int i = 0; i < bank->numberBranches; i++){
    pthread_mutex_lock(&bank->branches[i].lock);
  }
  for (unsigned int branch = 0; branch < bank->numberBranches; branch++) {
    // pthread_mutex_lock(&bank->branches[branch].lock);
    int berr = Branch_Validate(bank,bank->branches[branch].branchID);
    // pthread_mutex_unlock(&bank->branches[branch].lock);
    if (berr < 0) {
      err = berr;
    }
  }
  for(unsigned int i = 0; i < bank->numberBranches; i++){
    pthread_mutex_unlock(&bank->branches[i].lock);
  }
  return err;
}

/*
 * compare the data inside two banks and see they are exactly the same;
 * it is called in BankTest.
 */
int
Bank_Compare(Bank *bank1, Bank *bank2)
{
  int err = 0;
  if (bank1->numberBranches != bank2->numberBranches) {
    fprintf(stderr, "Bank num branches mismatch\n");
    return -1;
  }

  // for(unsigned int i = 0; i < bank2->numberBranches; i++){
  //   pthread_mutex_lock(&bank2->branches[i].lock);
  //   for(int j = 0; j < bank2->branches[i].numberAccounts; j++){
  //     pthread_mutex_lock(&bank2->branches[i].accounts[j].lock);
  //   }
  // }

  for (unsigned int branch = 0; branch < bank1->numberBranches; branch++) {
    // pthread_mutex_lock(&bank2->branches[branch].lock);
    int berr = Branch_Compare(&bank1->branches[branch],
                              &bank2->branches[branch]);
    // pthread_mutex_unlock(&bank2->branches[branch].lock);
    if (berr < 0) {
      err = berr;
    }
  }

  int cerr = Report_Compare(bank1, bank2);
  if (cerr < 0)
    err = cerr;

  // for(unsigned int i = 0; i < bank2->numberBranches; i++){
  //   for(int j = 0; j < bank2->branches[i].numberAccounts; j++){
  //     pthread_mutex_unlock(&bank2->branches[i].accounts[j].lock);
  //   }
  //   pthread_mutex_unlock(&bank2->branches[i].lock);
  // }

  return err;

}
