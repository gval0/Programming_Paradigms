#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "teller.h"
#include "account.h"
#include "error.h"
#include "debug.h"


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
  BranchID branchId = AccountNum_GetBranchID(accountNum);
  pthread_mutex_lock(&(bank->branches[branchId].branchLock));
  pthread_mutex_lock(&(account->accLock));
  if (account == NULL) {
    pthread_mutex_unlock(&(account->accLock));
    pthread_mutex_unlock(&(bank->branches[branchId].branchLock));
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  Account_Adjust(bank,account, amount, 1);
  pthread_mutex_unlock(&(account->accLock));
  pthread_mutex_unlock(&(bank->branches[branchId].branchLock));  
  return ERROR_SUCCESS;
}

/*
 * withdraw money from an account
 */
int
Teller_DoWithdraw(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{

  assert(amount >= 0);
  // hrere smth
  DPRINTF('t', ("Teller_DoWithdraw(account 0x%"PRIx64" amount %"PRId64")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);
  BranchID branchId = AccountNum_GetBranchID(accountNum);
  pthread_mutex_lock(&(bank->branches[branchId].branchLock));
  pthread_mutex_lock(&(account->accLock));

  if (account == NULL) {
    pthread_mutex_unlock(&(account->accLock));
    pthread_mutex_unlock(&(bank->branches[branchId].branchLock));
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  if (amount > Account_Balance(account)) {
    pthread_mutex_unlock(&(account->accLock));
    pthread_mutex_unlock(&(bank->branches[branchId].branchLock));
    return ERROR_INSUFFICIENT_FUNDS;
  }

  Account_Adjust(bank, account, -amount, 1);
  pthread_mutex_unlock(&(account->accLock));
  pthread_mutex_unlock(&(bank->branches[branchId].branchLock));
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
  
  if(srcAccount == dstAccount) return ERROR_SUCCESS;

  int isSame = Account_IsSameBranch(srcAccountNum, dstAccountNum);
  if(isSame){
    if(srcAccount->accountNumber < dstAccount->accountNumber){
      pthread_mutex_lock(&(srcAccount->accLock));
      pthread_mutex_lock(&(dstAccount->accLock));
    } else {
      pthread_mutex_lock(&(dstAccount->accLock));
      pthread_mutex_lock(&(srcAccount->accLock));
    }
    if (amount > Account_Balance(srcAccount)) {
      pthread_mutex_unlock(&(dstAccount->accLock));
      pthread_mutex_unlock(&(srcAccount->accLock));
      return ERROR_INSUFFICIENT_FUNDS;
    }
    int updateBranch = !Account_IsSameBranch(srcAccountNum, dstAccountNum);
    Account_Adjust(bank, srcAccount, -amount, updateBranch);
    Account_Adjust(bank, dstAccount, amount, updateBranch);
    pthread_mutex_unlock(&(dstAccount->accLock));
    pthread_mutex_unlock(&(srcAccount->accLock));
    return ERROR_SUCCESS;
  } else {
  BranchID scrBranch = AccountNum_GetBranchID(srcAccountNum);
  BranchID dstBranch = AccountNum_GetBranchID(dstAccountNum);
    if(scrBranch < dstBranch){
      pthread_mutex_lock(&(bank->branches[scrBranch].branchLock));
      pthread_mutex_lock(&(bank->branches[dstBranch].branchLock));
      pthread_mutex_lock(&(srcAccount->accLock));
      pthread_mutex_lock(&(dstAccount->accLock));
    } else {
      pthread_mutex_lock(&(bank->branches[dstBranch].branchLock));
      pthread_mutex_lock(&(bank->branches[scrBranch].branchLock));
      pthread_mutex_lock(&(dstAccount->accLock));
      pthread_mutex_lock(&(srcAccount->accLock));
    }
    if (amount > Account_Balance(srcAccount)) {
      pthread_mutex_unlock(&(dstAccount->accLock));
      pthread_mutex_unlock(&(srcAccount->accLock));
      pthread_mutex_unlock(&(bank->branches[dstBranch].branchLock));
      pthread_mutex_unlock(&(bank->branches[scrBranch].branchLock));
      return ERROR_INSUFFICIENT_FUNDS;
    }
    int updateBranch = !Account_IsSameBranch(srcAccountNum, dstAccountNum);
    Account_Adjust(bank, srcAccount, -amount, updateBranch);
    Account_Adjust(bank, dstAccount, amount, updateBranch);
    pthread_mutex_unlock(&(dstAccount->accLock));
    pthread_mutex_unlock(&(srcAccount->accLock));
    pthread_mutex_unlock(&(bank->branches[dstBranch].branchLock));
    pthread_mutex_unlock(&(bank->branches[scrBranch].branchLock));
    return ERROR_SUCCESS;
  }
}
