
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include "teller.h"
#include "account.h"
#include "error.h"
#include "debug.h"

#include "branch.h"
#include "report.h"

#include <pthread.h>

/*
 * initialize the account based on the passed-in information.
 */
void
Account_Init(Bank *bank, Account *account, int id, int branch,
             AccountAmount initialAmount)
{
  extern int testfailurecode;

  account->accountNumber = Account_MakeAccountNum(branch, id);

  pthread_mutex_init(&account->lock, NULL);

  // pthread_mutex_lock(&account->lock);
  account->balance = initialAmount;
  // pthread_mutex_unlock(&account->lock);

  
  if (testfailurecode) {
    // To test failures, we initialize every 4th account with a negative value
    if ((id & 0x3) == 0) {
      // pthread_mutex_lock(&account->lock);
      account->balance = -1;
      // pthread_mutex_unlock(&account->lock);
    }
  }
}

/*
 * get the ID of the branch which the account is in.
 */
BranchID
AccountNum_GetBranchID(AccountNumber accountNum)
{
  Y;
  return (BranchID) (accountNum >> 32);
}

/*
 * get the branch-wide subaccount number of the account.
 */
int
AcountNum_Subaccount(AccountNumber accountNum)
{
  Y;
  return  (accountNum & 0x7ffffff);
}

/*
 * find the account address based on the accountNum.
 */
Account *
Account_LookupByNumber(Bank *bank, AccountNumber accountNum)
{
  BranchID branchID =  AccountNum_GetBranchID(accountNum);
  int branchIndex = AcountNum_Subaccount(accountNum);
  return &(bank->branches[branchID].accounts[branchIndex]);
}

/*
 * adjust the balance of the account.
 */
void
Account_Adjust(Bank *bank, Account *account, AccountAmount amount,
               int updateBranch)
{

  // pthread_mutex_lock(&account->lock);
  AccountAmount oldBalance = account->balance;
  account->balance = oldBalance + amount;
  // pthread_mutex_unlock(&account->lock);
  
  if (updateBranch) {
    Branch_UpdateBalance(bank, AccountNum_GetBranchID(account->accountNumber), amount);                  
  } Y;
}
/*
 * return the balance of the account.
 */
AccountAmount
Account_Balance(Account *account)
{
  // pthread_mutex_lock(&account->lock);
  AccountAmount balance = account->balance; Y;
  // pthread_mutex_unlock(&account->lock);
  return balance;
}

/*
 * make the account number based on the branch number and
 * the branch-wise subaccount number.
 */
AccountNumber
Account_MakeAccountNum(int branch, int subaccount)
{
  AccountNumber num;

  num = subaccount;
  num |= ((uint64_t) branch) << 32;  Y;
  return num;
}

/*
 * Test to see if two accounts are in the same branch.
 */

int
Account_IsSameBranch(AccountNumber accountNum1, AccountNumber accountNum2)
{
  return (AccountNum_GetBranchID(accountNum1) ==
          AccountNum_GetBranchID(accountNum2));
}
