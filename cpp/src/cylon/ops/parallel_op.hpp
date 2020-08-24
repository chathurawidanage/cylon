#ifndef CYLON_SRC_CYLON_OPS_PARALLEL_OP_HPP_
#define CYLON_SRC_CYLON_OPS_PARALLEL_OP_HPP_

#include <memory>
#include <table.hpp>

/**
 * Assumptions
 * 1. Queue, Map lookups will never fail
 *
 * todo some functions can be final. Couldn't find a alternative for Java's final methods yet
 */
namespace cylon {
class ResultsCallback {
 public:
  virtual void OnResult(int tag, std::shared_ptr<cylon::Table> table) = 0;
};

class Op {
 private:
  int id;
  std::unordered_map<int, std::queue<std::shared_ptr<cylon::Table>> *> queues{};

  // this count should be increased when adding table to the queues,
  // and should be decrease when removing a table from the queue
  int inputs_count = 0;
  std::unordered_map<int, cylon::Op *> children{};
  std::shared_ptr<ResultsCallback> callback;
  std::function<int(int)> router;
  std::shared_ptr<arrow::Schema> schema;

  std::queue<std::shared_ptr<cylon::Table>> *GetQueue(int tag);
  Op *GetChild(int tag);

  // capturing the parent status
  int32_t parents = 0;
  int32_t finalized_parents = 0;

  // finalize status
  bool all_parents_finalized = false;
  bool finalized = false;

  /**
   * This function depends on inputs_count to determine whether Op has any input data to proceed
   * @return
   */
  bool Ready();

  /**
   * If this Op is a leaf op, this function will send the table to the callback. todo this can be private?
   * @param tag tag of the table
   * @param table pointer to the table
   * @return true if this op is a leaf, false otherwise
   */
  bool TerminalCheck(int tag, std::shared_ptr<cylon::Table> table);

 protected:
  std::shared_ptr<cylon::CylonContext> ctx_;

  /**
   * Parent Op will call this method on (this)child when this has been added a child to the parent Op
   */
  void IncrementParentsCount();

  /**
   * Parent Op will call this method to report it's completion to the child
   */
  void ReportParentCompleted();

  /**
   * This function can be used if this Op is just a forwarding Op. todo this is a util, possible to move outside the class
   * @param queue queue Id
   * @param child child Id
   * @param tag tag to be sent with the tables
   */
  void DrainQueueToChild(int queue, int child, int tag);

  /**
  * Inserts the given table to the input Queue of every child todo this is a util, possible to move outside the class
  * @param tag tag of the table
  * @param table pointer to the table
  */
  void InsertToAllChildren(int tag, std::shared_ptr<cylon::Table> table);

  /**
   * Inserts the table to the input queue of a specific child todo this is a util, possible to move outside the class
   * @param tag tag of the table
   * @param child target child ID
   * @param table pointer to the table
   */
  void InsertToChild(int tag, int child, std::shared_ptr<cylon::Table> table);

 public:
  Op(std::shared_ptr<cylon::CylonContext> ctx,
     std::shared_ptr<arrow::Schema> schema,
     int id,
     std::shared_ptr<ResultsCallback> callback, bool root_op = false);

  /**
   * This function can be used to link a child Op to parent Op
   * @param child child op to be linked
   * @return return the parent Op instance, for chaining
   */
  cylon::Op *AddChild(cylon::Op *child);

  /**
   * This function will be used by parent Ops to insert a table to the input queue.
   * @param tag
   * @param table
   */
  void InsertTable(int tag, std::shared_ptr<cylon::Table> table);

  /**
   * This function defines the execution logic of this op. It can be either a computation or a communication
   * @param tag
   * @param table
   * @return True if this table has been processed, False if this table is partially processed. If True is returned,
   * same table will be sent in the next iteration. State can be saved local to the Op, if partially processed.
   */
  virtual bool Execute(int tag, std::shared_ptr<Table> table) = 0;

  /**
   * This is where this Op get's CPU time. Progress will do following things in order.
   *
   * 1) Check whether this Op has anything to process. doing Read() call
   * 2) If there are anything left to process, call Execute()
   * 3) If Execute() returns true, remove the table from the queue, else keep it in the queue
   * 4) Then check if this Op can be finalized, based on following conditions.
   *    a) No more inputs to process
   *    b) All parents have finalized
   * 5) If this Op is finalizable call Finalize()
   * 6) Call progress in Child Ops(Child branches)
   *
   * This function can be overridden in child Ops, but it's mandatory to call Op::Progress in such cases
   */
  virtual void Progress();

  /**
   * Parent Op is considered to be completed only if all child Ops are completed
   * @return
   */
  bool IsComplete();

  /**
   * This function will be called when no more inputs will be received to this Op, ie parents have been finalized
   */
  virtual void OnParentsFinalized() = 0;

  /**
   * Every op should implement this function. This function will be called by Progress()
   * when the end of Op life is reached.
   */
  virtual bool Finalize() = 0;

  ~Op();
};
}
#endif //CYLON_SRC_CYLON_OPS_PARALLEL_OP_HPP_
