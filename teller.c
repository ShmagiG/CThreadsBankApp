#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "teller.h"
#include "account.h"
#include "error.h"
#include "debug.h"

#include <pthread.h>
#include "branch.h"
#include "account.c"


typedef uint64_t BranchID;
/*
 * deposit money into an account
 */
int
Teller_DoDeposit(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoDeposit(account 0x%"PRIx64" amount %"PRId64")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);

  if (account == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  pthread_mutex_lock(&bank->branches[AccountNum_GetBranchID(accountNum)].lock);
  pthread_mutex_lock(&account->lock);
  Account_Adjust(bank,account, amount, 1);
  pthread_mutex_unlock(&account->lock);
  pthread_mutex_unlock(&bank->branches[AccountNum_GetBranchID(accountNum)].lock);

  return ERROR_SUCCESS;
}

/*
 * withdraw money from an account
 */
int
Teller_DoWithdraw(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoWithdraw(account 0x%"PRIx64" amount %"PRId64")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);
  pthread_mutex_lock(&bank->branches[AccountNum_GetBranchID(accountNum)].lock);
  pthread_mutex_lock(&account->lock);

  if (account == NULL) {
    pthread_mutex_unlock(&account->lock);
    pthread_mutex_unlock(&bank->branches[AccountNum_GetBranchID(accountNum)].lock);
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  if (amount > Account_Balance(account)) {
    pthread_mutex_unlock(&account->lock);
    pthread_mutex_unlock(&bank->branches[AccountNum_GetBranchID(accountNum)].lock);
    return ERROR_INSUFFICIENT_FUNDS;
  }
  
  Account_Adjust(bank,account, -amount, 1);
  pthread_mutex_unlock(&account->lock);
  pthread_mutex_unlock(&bank->branches[AccountNum_GetBranchID(accountNum)].lock);

  return ERROR_SUCCESS;
}

/*
 * do a tranfer from one account to another account
 */
int
Teller_DoTransfer(Bank *bank, AccountNumber srcAccountNum,
                  AccountNumber dstAccountNum,
                  AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoTransfer(src 0x%"PRIx64", dst 0x%"PRIx64
                ", amount %"PRId64")\n",
                srcAccountNum, dstAccountNum, amount));

  Account *srcAccount = Account_LookupByNumber(bank, srcAccountNum);
  

  if (srcAccount == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  Account *dstAccount = Account_LookupByNumber(bank, dstAccountNum);
  if (dstAccount == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }
  /*
   * If we are doing a transfer within the branch, we tell the Account module to
   * not bother updating the branch balance since the net change for the
   * branch is 0.
   */
  int updateBranch = !Account_IsSameBranch(srcAccountNum, dstAccountNum);
  pthread_mutex_lock(&(bank->branches[AccountNum_GetBranchID(srcAccount->accountNumber)].lock));
  pthread_mutex_lock(&srcAccount->lock);
  if (amount > Account_Balance(srcAccount)) {
    pthread_mutex_unlock(&srcAccount->lock);
    pthread_mutex_unlock(&(bank->branches[AccountNum_GetBranchID(srcAccount->accountNumber)].lock));
    return ERROR_INSUFFICIENT_FUNDS;
  }
  Account_Adjust(bank, srcAccount, -amount, 1);
  pthread_mutex_unlock(&srcAccount->lock);
  pthread_mutex_unlock(&(bank->branches[AccountNum_GetBranchID(srcAccount->accountNumber)].lock));

  pthread_mutex_lock(&(bank->branches[AccountNum_GetBranchID(dstAccount->accountNumber)].lock));
  pthread_mutex_lock(&dstAccount->lock);
  Account_Adjust(bank, dstAccount, amount, 1);  
  pthread_mutex_unlock(&dstAccount->lock);
  pthread_mutex_unlock(&(bank->branches[AccountNum_GetBranchID(dstAccount->accountNumber)].lock));

  return ERROR_SUCCESS;
}
