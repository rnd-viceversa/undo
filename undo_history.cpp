// Undo Library
// Copyright (C) 2015-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "undo_history.h"

#include "undo_command.h"
#include "undo_state.h"

#include <cassert>
#include <stack>

#define UNDO_TRACE(...)

namespace undo {

UndoHistory::UndoHistory(std::shared_ptr<UndoHistoryDelegate> delegate)
  : m_delegate(delegate)
  , m_first(nullptr)
  , m_last(nullptr)
  , m_cur(nullptr)
{
}

UndoHistory::~UndoHistory()
{
  m_cur = nullptr;
  clearRedo();
}

bool UndoHistory::canUndo() const
{
  return m_cur != nullptr;
}

bool UndoHistory::canRedo() const
{
  return m_cur != m_last;
}

void UndoHistory::undo()
{
  assert(m_cur);
  if (!m_cur)
    return;

  assert(
    (m_cur != m_first && m_cur->m_prev) ||
    (m_cur == m_first && !m_cur->m_prev));

  moveTo(m_cur->m_prev);
}

void UndoHistory::redo()
{
  if (!m_cur)
    moveTo(m_first);
  else
    moveTo(m_cur->m_next);
}

void UndoHistory::clearRedo()
{
  if (m_cur) {
    m_cur->m_next = nullptr;
    m_last = m_cur;
  }
  else {
    m_first = m_last = nullptr;
  }
}

bool UndoHistory::deleteFirstState()
{
  UNDO_TRACE("UndoHistory::deleteFirstState()\n");

  // We cannot delete the first state if we are in the first state.
  if (m_cur == m_first) {
    UNDO_TRACE(" - Cannot delete first state if it's the current state\n");
    return false;
  }

  std::shared_ptr<UndoState> i = m_last;
  while (i) {
    // If this state depends on the delete one, this "i" is the new
    // m_first undo state.
    if (i->m_parent == m_first) {
      // First we check if the current undo state is one of the states
      // that we're going to delete.
        std::shared_ptr<UndoState> j = m_first;
      while (j != i) {
        // Cannot delete this "j" state because is the current one.
        if (m_cur == j) {
          UNDO_TRACE(" - Cannot delete first state because current state depends on it to go to the last state\n");
          return false;
        }
        j = j->next();
      }

      j = m_first;
      while (j != i) {
        UNDO_TRACE(" - Delete undo state\n");

        std::shared_ptr<UndoState> k = j;
        j = j->next();

        deleteState(k);
      }

      i->m_prev = nullptr;
      i->m_parent = nullptr;
      std::shared_ptr<UndoState> temp(i);
      m_first = temp;
      return true;
    }
    i = i->prev();
  }

  std::shared_ptr<UndoState> state = m_first;
  assert(m_last == m_first);
  assert(m_first->next() == nullptr);
  m_first = m_last = nullptr;
  UNDO_TRACE(" - Delete first state only\n");

  deleteState(state);
  return true;
}

void UndoHistory::add(std::shared_ptr<UndoCommand> cmd)
{
  std::shared_ptr<UndoState> state = std::make_shared<UndoState>(cmd);
  state->m_prev = m_last;
  state->m_next = nullptr;
  state->m_parent = m_cur;

  if (!m_first)
    m_first = state;

  m_cur = m_last = state;

  if (state->m_prev) {
    assert(!state->m_prev->m_next);
    state->m_prev->m_next = state;
  }
}


std::shared_ptr<UndoState> UndoHistory::findCommonParent(
    const std::shared_ptr<UndoState> a, const std::shared_ptr<UndoState> b)
{
  std::shared_ptr<UndoState> pA = a;
  std::shared_ptr<UndoState> pB = b;

  if (pA == nullptr || pB == nullptr)
    return nullptr;

  while (pA != pB) {
    pA = pA->m_parent;
    if (!pA) {
      pA = a;
      pB = pB->m_parent;
      if (!pB)
        return nullptr;
    }
  }

  return pA;
}

void UndoHistory::moveTo(const std::shared_ptr<UndoState> new_state)
{
    std::shared_ptr<UndoState> common = findCommonParent(m_cur, new_state);

  if (m_cur) {
    while (m_cur != common) {
      m_cur->m_cmd->undo();
      m_cur = m_cur->m_parent;
    }
  }

  if (new_state) {
    std::stack<std::shared_ptr<UndoState>> redoParents;
    std::shared_ptr<UndoState> p = new_state;
    while (p != common) {
        redoParents.push(p);
      p = p->m_parent;
    }

    while (!redoParents.empty()) {
      p = redoParents.top();
      redoParents.pop();

      p->m_cmd->redo();
    }
  }

  m_cur = new_state;
}

void UndoHistory::deleteState(std::shared_ptr<UndoState> state)
{
  if (m_delegate)
    m_delegate->onDeleteUndoState(state);
}

} // namespace undo
