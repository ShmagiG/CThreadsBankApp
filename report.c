#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>


#include "error.h"
#include "debug.h"

#include "bank.h"
#include "branch.h"
#include "account.h"
#include "report.h"

#include <pthread.h>




static AccountAmount reportingAmount;   // Reporting threshold amount
static int numWorkers;                  // Number of worker threads in the system
 
/*
 * Initialize the Report module of a bank.  Returns -1 on an error, 0 otherwise.
 */
int
Report_Init(Bank *bank, AccountAmount reportAmount, int maxNumWorkers)
{
  // Allocate and fill in the Report structure of the bank.

  // pthread_mutex_lock(&bank->lock);
  bank->report = (Report *) malloc(sizeof(Report));
  // pthread_mutex_unlock(&bank->lock);
  if (bank->report == NULL) {
    return -1;
  }

  pthread_mutex_init(&bank->report->lock, NULL);


  bank->report->numReports = 0;

  for (int r = 0; r < MAX_NUM_REPORTS; r++) {
    bank->report->dailyData[r].hasOverflowed = 0;
    bank->report->dailyData[r].numLogEntries = 0;
  }

  // pthread_mutex_unlock(&bank->report->lock);

  // Save the reporting amount and number of workers for this module to use.
  reportingAmount = reportAmount;
  numWorkers = maxNumWorkers; 

  return 0;
}


/*
 * Report a transfer to/from the specified accountNum in the amount of amount.
 * This is called for all tranfers but we need record only the ones that
 * are at or above the reporting amount. The worker making the call is
 * given to us (workerNum). Returns 0 on success, non-zero otherwise.
 */
int
Report_Transfer(Bank *bank, int workerNum, AccountNumber accountNum,
                AccountAmount amount)
{
  // Compute the absolute amount of the transfer; withdrawals come in as negative numbers.
  pthread_mutex_lock(&bank->report->lock);
  Report *rpt = bank->report;
  int r  = rpt->numReports; Y;

  AccountAmount amountAbs = (amount < 0) ? -amount : amount;   Y;
  if (amountAbs < reportingAmount) {
    pthread_mutex_unlock(&(bank->report->lock));
    return 0;  // Too small to report.
  }
  
  
  // int r  = rpt->numReports; Y;

  if (r >= MAX_NUM_REPORTS) {
      // We've run out of report storage for the bank
      pthread_mutex_unlock(&(bank->report->lock));
      return 0;
  }

  if (rpt->dailyData[r].numLogEntries >= MAX_LOG_ENTRIES) {
    // Current report is full, mark it as overflowed and return.
    rpt->dailyData[r].hasOverflowed = 1;\
    pthread_mutex_unlock(&(bank->report->lock));
    return 0;
  }
  // Add the record to the end of the log of records.
  int ent = rpt->dailyData[r].numLogEntries; Y;
  rpt->dailyData[r].transferLog[ent].accountNum = accountNum; Y;
  rpt->dailyData[r].transferLog[ent].transferSize = amount;   Y;
  rpt->dailyData[r].numLogEntries = ent + 1; Y;

  pthread_mutex_unlock(&(bank->report->lock));
  return 0;
}

/*
 * Perform the nightly report. Is called by every worker for each report period. workerNum is
 * the worker making the call.  Returns -1 on error, 0 otherwise.
 */
int
Report_DoReport(Bank *bank, int workerNum)
{
  pthread_mutex_lock(&bank->report->lock);
  bank->numWorking --;
  // pthread_mutex_lock(&bank->doReportLock[workerNum]);
  pthread_mutex_lock(&bank->doLock);
  if(bank->numWorking != 0){
    pthread_mutex_unlock(&bank->report->lock);
    
    
    pthread_cond_wait(&bank->reportCond, &bank->doLock);
    pthread_mutex_unlock(&bank->doLock);
    return 0;
  }
  Report *rpt = bank->report;
  // pthread_mutex_lock(&rpt->lock);
  assert(rpt); Y;

  if (rpt->numReports >= MAX_NUM_REPORTS) {
      // We've run out of report storage for the bank
      // pthread_mutex_unlock(&rpt->lock);
      bank->numWorking = numWorkers;
      pthread_mutex_unlock(&bank->report->lock);
      return -1;
  }

  /*
   * Store the overall bank balance for the report.
   */
  int err = Bank_Balance(bank, &rpt->dailyData[rpt->numReports].balance); Y;
  int oldNumReports = rpt->numReports; Y;
  rpt->numReports = oldNumReports + 1; Y;
  // pthread_mutex_unlock(&rpt->lock); 
  pthread_cond_broadcast(&bank->reportCond);
  pthread_mutex_unlock(&bank->doLock);
  bank->numWorking = numWorkers;
  pthread_mutex_unlock(&bank->report->lock);
  return err;
}


/*
 *
 * Function used by qsort() to record log.
 */
static int
TransferLogSortFunc(const void *p1, const void *p2)
{
  const struct TransferLog *l1 = (const struct TransferLog *) p1;
  const struct TransferLog *l2 = (const struct TransferLog *) p2;

  if (l1->accountNum < l2->accountNum) return -1;
  if (l1->accountNum > l2->accountNum) return 1;
  if (l1->transferSize < l2->transferSize) return -1;
  if (l1->transferSize > l2->transferSize) return 1;

  return 0;
}

/*
 * Compare the report data inside two banks and see they are exactly the same;
 * Prints mismatches to stderr,
 * Return -1 on mismatch, zero otherwise.
 */
int
Report_Compare(Bank *bank1, Bank *bank2)
{
  int err = 0;

  Report *rpt1 = bank1->report;
  Report *rpt2 = bank2->report;

  if (rpt1->numReports != rpt2->numReports) {
    fprintf(stderr, "Bank num reports mismatch %d != %d\n",
            rpt1->numReports, rpt2->numReports);
    err = -1;
  }


  for (int r = 0; r < rpt1->numReports; r++) {
    // pthread_mutex_lock(&rpt2->dailyData[r].lock);
    if (rpt1->dailyData[r].balance != rpt2->dailyData[r].balance) {
      fprintf(stderr, "Report %d for banks mismatch %"PRId64" and %"PRId64"\n",
              r, rpt1->dailyData[r].balance, rpt2->dailyData[r].balance);
      err = -1;
    }
    if (rpt1->dailyData[r].numLogEntries !=  rpt2->dailyData[r].numLogEntries) {
      fprintf(stderr, "Report different number of log entries (%d and %d)\n",
              rpt1->dailyData[r].numLogEntries,
              rpt2->dailyData[r].numLogEntries);
      // pthread_mutex_unlock(&rpt2->dailyData[r].lock);
      return -1;
    }
    if (!rpt1->dailyData[r].hasOverflowed) {
      int i, n;
      // If the transfer log hasn't overflowed we can compare the log. We should get the
      // same log entries but possibly in a different order. To account for order we sort
      // the logs.

      n = rpt1->dailyData[r].numLogEntries;
      qsort(rpt1->dailyData[r].transferLog, n, sizeof(struct TransferLog),
            TransferLogSortFunc);

      assert(n == rpt2->dailyData[r].numLogEntries);
      qsort(rpt2->dailyData[r].transferLog, n, sizeof(struct TransferLog),
            TransferLogSortFunc);

      for (i = 0; i < n; i++) {
        if ((rpt1->dailyData[r].transferLog[i].accountNum !=
             rpt2->dailyData[r].transferLog[i].accountNum) ||
            (rpt1->dailyData[r].transferLog[i].transferSize !=
             rpt2->dailyData[r].transferLog[i].transferSize)) {
          fprintf(stderr, "Report transferLog %d difference at %d\n", r, i);
          err = -1;
        }
      }
    }
    // pthread_mutex_unlock(&rpt2->dailyData[r].lock);
  }

  return err;

}

