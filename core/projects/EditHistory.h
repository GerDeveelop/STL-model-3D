#pragma once
#include "core/cutting/PlaneCut.h"
#include <memory>
#include <cstdint>
#include <string>
#include <vector>

namespace stlcut {
using PieceId=std::uint64_t;
struct Piece {
    PieceId id{},parentId{};
    std::string name,parentName;
    std::shared_ptr<const Mesh> mesh;
    std::uint64_t presentationBytes{};
};
using PiecePtr=std::shared_ptr<const Piece>;
struct EditState {
    std::vector<PiecePtr> pieces;
    PieceId selected{};
    Plane plane;
    bool planeVisible{true};
    ConnectorSpec connector;
    bool connectorEnabled{};
};
struct HistoryLimits {
    std::uint64_t bytes=1024ULL*1024*1024;
    size_t cuts=32,pieces=256;
};
struct CutCommit { PieceId positive{},negative{};size_t discardedSteps{}; };

// Session-only history. Geometry is immutable/shared; selection/plane edits are
// part of the current state, while only successful cuts create undo entries.
class EditHistory {
public:
    explicit EditHistory(HistoryLimits limits={});
    void reset(std::shared_ptr<const Mesh> original,std::string name,std::uint64_t presentationBytes=0);
    bool empty() const {return !original_;}
    const EditState& state() const;
    const PiecePtr& original() const {return original_;}
    PiecePtr selected() const;
    bool selectedIsActive() const;
    void select(PieceId id);
    void setPlane(Plane plane,bool visible=true);
    void setConnector(ConnectorSpec connector,bool enabled);
    CutCommit commitCut(std::shared_ptr<const Mesh> positive,std::shared_ptr<const Mesh> negative,
                        std::uint64_t positivePresentationBytes=0,std::uint64_t negativePresentationBytes=0);
    std::vector<PieceId> commitSplit(const std::vector<std::shared_ptr<const Mesh>>& pieces,
                                    const std::vector<std::uint64_t>& presentationBytes = {});
    bool undo();
    bool redo();
    size_t undoCount() const {return cursor_;}
    size_t redoCount() const {return states_.empty()?0:states_.size()-cursor_-1;}
    std::uint64_t retainedBytes() const;
    std::vector<PieceId> retainedIds() const;
private:
    std::uint64_t retainedBytes(const std::vector<EditState>& states,const PiecePtr& root) const;
    HistoryLimits limits_;
    PiecePtr original_;
    std::vector<EditState> states_;
    size_t cursor_{};
    PieceId nextId_{2};
};
}
