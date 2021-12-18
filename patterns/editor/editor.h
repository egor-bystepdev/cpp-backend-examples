#pragma once

#include <string>
#include <memory>
#include <vector>
#include <iostream>

class Command;

class Editor {
private:
public:
    Editor() = default;

    const std::string& GetText() const;

    void Type(char c);

    void ShiftLeft();

    void ShiftRight();

    void Backspace();

    void Undo();

    void Redo();

private:
    void CommandAggregate(std::unique_ptr<Command> cmd);

    std::string text_ = std::string();
    size_t cursor_ = 0;
    std::vector<std::unique_ptr<Command>> last_;
    size_t action_now_index_ = 0;

    friend class CommandType;
    friend class CommandBackspace;
    friend class CommandShiftLeft;
    friend class CommandShiftRight;
};

class Command {
public:
    Command(Editor& edit) : editor(edit) {
    }
    virtual ~Command() = default;

    virtual bool Do() = 0;
    virtual void Undo() = 0;

protected:
    Editor& editor;
};

class CommandType : public Command {
public:
    CommandType(Editor& edit, char c) : Command(edit), c_(c) {
    }

    bool Do() override {
        editor.text_.insert(editor.cursor_++, 1, c_);
        return true;
    }

    void Undo() override {
        editor.text_.erase(--editor.cursor_, 1);
    }

private:
    char c_;
};

class CommandBackspace : public Command {
public:
    CommandBackspace(Editor& edit) : Command(edit) {
    }

    bool Do() override {
        if (editor.cursor_) {
            c_ = editor.text_[--editor.cursor_];
            editor.text_.erase(editor.cursor_, 1);
            return true;
        }
        return false;
    }

    void Undo() override {
        editor.text_.insert(editor.cursor_++, 1, c_);
    }

private:
    char c_;
};

class CommandShiftLeft : public Command {
public:
    CommandShiftLeft(Editor& edit) : Command(edit) {
    }

    bool Do() override {
        if (editor.cursor_) {
            --editor.cursor_;
            return true;
        }
        return false;
    }

    void Undo() override {
        ++editor.cursor_;
    }
};

class CommandShiftRight : public Command {
public:
    CommandShiftRight(Editor& edit) : Command(edit) {
    }

    bool Do() override {
        if (editor.cursor_ < editor.text_.size()) {
            ++editor.cursor_;
            return true;
        }
        return false;
    }

    void Undo() override {
        --editor.cursor_;
    }
};

const std::string& Editor::GetText() const {
    return text_;
}

void Editor::Type(char c) {
    CommandAggregate(std::unique_ptr<Command>(new CommandType(*this, c)));
}

void Editor::CommandAggregate(std::unique_ptr<Command> cmd) {
    if (cmd->Do()) {
        last_.erase(last_.begin() + action_now_index_, last_.end());
        last_.emplace_back(std::move(cmd));
        action_now_index_ = last_.size();
    }
}

void Editor::ShiftLeft() {
    CommandAggregate(std::unique_ptr<Command>(new CommandShiftLeft(*this)));
}

void Editor::ShiftRight() {
    CommandAggregate(std::unique_ptr<Command>(new CommandShiftRight(*this)));
}

void Editor::Backspace() {
    CommandAggregate(std::unique_ptr<Command>(new CommandBackspace(*this)));
}

void Editor::Undo() {
    if (action_now_index_) {
        last_[--action_now_index_]->Undo();
    }
}

void Editor::Redo() {
    if (action_now_index_ != last_.size()) {
        last_[action_now_index_++]->Do();
    }
}
