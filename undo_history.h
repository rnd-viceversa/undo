// Undo Library
// Copyright (C) 2015-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UNDO_HISTORY_H_INCLUDED
#define UNDO_HISTORY_H_INCLUDED
#pragma once

#include <memory>

namespace undo {

  class UndoCommand;
  class UndoState;

  class UndoHistoryDelegate {
  public:
    virtual ~UndoHistoryDelegate() { }
    virtual void onDeleteUndoState(std::shared_ptr<UndoState> state) { }
  };

  class UndoHistory {
  public:
    UndoHistory(std::shared_ptr<UndoHistoryDelegate> delegate = nullptr);
    virtual ~UndoHistory();

    const std::shared_ptr<UndoState> firstState()   const { return m_first; }
    const std::shared_ptr<UndoState> lastState()    const { return m_last; }
    const std::shared_ptr<UndoState> currentState() const { return m_cur; }

    void add(std::shared_ptr<UndoCommand> cmd);
    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();

    // Deletes the whole redo history. Can be called before an add()
    // to create a linear undo history.
    void clearRedo();

    // Deletes the first UndoState. It can be useful to limit the size
    // of the undo history.
    bool deleteFirstState();

    // This can be used to jump to a specific UndoState in the whole
    // history.
    void moveTo(const std::shared_ptr<UndoState> new_state);

  private:
    static std::shared_ptr<UndoState> findCommonParent(
        const std::shared_ptr<UndoState> a, const std::shared_ptr<UndoState> b);
    void deleteState(std::shared_ptr<UndoState> state);

    std::shared_ptr<UndoHistoryDelegate> m_delegate;
    std::shared_ptr<UndoState> m_first;
    std::shared_ptr<UndoState> m_last;
    std::shared_ptr<UndoState> m_cur; // Current action that can be undone
  };

} // namespace undo

#endif  // HISTORY_H_INCLUDED
