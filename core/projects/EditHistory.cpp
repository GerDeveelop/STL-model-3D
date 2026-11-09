#include "EditHistory.h"
#include "core/io/StlReader.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <set>
#include <stdexcept>

namespace stlcut {
namespace {
void validMesh(const std::shared_ptr<const Mesh>& mesh) {
    if(!mesh||mesh->triangles.empty()||mesh->triangles.size()>maxTriangles)
        throw std::runtime_error("Pieza vacia o fuera del limite de triangulos.");
}
}
EditHistory::EditHistory(HistoryLimits limits):limits_(limits) {
    if(!limits.bytes||limits.bytes>1024ULL*1024*1024||!limits.cuts||limits.cuts>32||!limits.pieces||limits.pieces>256)
        throw std::runtime_error("Limites de historial invalidos (maximo 1 GiB, 32 pasos y 256 piezas).");
}
const EditState& EditHistory::state() const {
    if(empty())throw std::runtime_error("No hay modelo en la sesion.");
    return states_.at(cursor_);
}
std::uint64_t EditHistory::retainedBytes(const std::vector<EditState>& states,const PiecePtr& root) const {
    std::set<const Mesh*> meshes;
    std::set<const Piece*> pieces;
    std::uint64_t bytes=0;
    auto add=[&](std::uint64_t value) {
        if(value>std::numeric_limits<std::uint64_t>::max()-bytes)throw std::runtime_error("Tamano de historial fuera de rango.");
        bytes+=value;
    };
    auto piece=[&](const PiecePtr& p) {
        if(!pieces.insert(p.get()).second)return;
        add(sizeof(Piece)+p->name.capacity()+p->parentName.capacity()+p->presentationBytes);
        if(meshes.insert(p->mesh.get()).second) {
            add(sizeof(Mesh)+p->mesh->triangles.capacity()*sizeof(Triangle)+p->mesh->units.capacity()+p->mesh->format.capacity());
            for(const auto& warning:p->mesh->warnings)add(warning.capacity());
        }
    };
    if(root)piece(root);
    for(const auto& s:states) {
        add(sizeof(EditState)+s.pieces.capacity()*sizeof(PiecePtr));
        for(const auto& p:s.pieces)piece(p);
    }
    return bytes;
}
std::uint64_t EditHistory::retainedBytes() const {return retainedBytes(states_,original_);}
std::vector<PieceId> EditHistory::retainedIds() const {
    std::set<PieceId> ids;
    if(original_)ids.insert(original_->id);
    for(const auto& s:states_)for(const auto& p:s.pieces)ids.insert(p->id);
    return {ids.begin(),ids.end()};
}
void EditHistory::reset(std::shared_ptr<const Mesh> mesh,std::string name,std::uint64_t presentationBytes) {
    validMesh(mesh);
    auto root=std::make_shared<const Piece>(Piece{1,0,std::move(name),{},std::move(mesh),presentationBytes});
    const auto& bounds=root->mesh->bounds;
    EditState initial{{root},1,{{0,0,1},std::midpoint(bounds.min.z,bounds.max.z)},true};
    std::vector<EditState> states{std::move(initial)};
    if(retainedBytes(states,root)>limits_.bytes)throw std::runtime_error("El modelo supera el presupuesto del historial; se conserva la sesion anterior.");
    original_=std::move(root);states_.swap(states);cursor_=0;nextId_=2;
}
PiecePtr EditHistory::selected() const {
    const auto& s=state();
    for(const auto& p:s.pieces)if(p->id==s.selected)return p;
    if(original_&&s.selected==original_->id)return original_;
    throw std::runtime_error("Seleccion de pieza no valida.");
}
bool EditHistory::selectedIsActive() const {
    if(empty())return false;
    const auto& s=state();
    return std::any_of(s.pieces.begin(),s.pieces.end(),[&](const auto& p){return p->id==s.selected;});
}
void EditHistory::select(PieceId id) {
    const auto& s=state();
    if(id!=original_->id&&!std::any_of(s.pieces.begin(),s.pieces.end(),[&](const auto& p){return p->id==id;}))
        throw std::runtime_error("La pieza no pertenece al estado actual.");
    states_[cursor_].selected=id;
}
void EditHistory::setPlane(Plane plane,bool visible) {
    state();states_[cursor_].plane=plane;states_[cursor_].planeVisible=visible;
}
CutCommit EditHistory::commitCut(std::shared_ptr<const Mesh> positive,std::shared_ptr<const Mesh> negative,
                                std::uint64_t positiveBytes,std::uint64_t negativeBytes) {
    if(!selectedIsActive())throw std::runtime_error("El original es una referencia; seleccione una pieza activa para cortar.");
    validMesh(positive);validMesh(negative);
    const auto& before=state();
    if(before.pieces.size()>=limits_.pieces)throw std::runtime_error("Se alcanzo el limite de piezas activas; el corte no se aplica.");
    if(nextId_>=std::numeric_limits<PieceId>::max()-1)throw std::runtime_error("Identificadores de pieza agotados.");
    const auto parent=selected();
    auto plus=std::make_shared<const Piece>(Piece{nextId_,parent->id,"Pieza "+std::to_string(nextId_)+" (+)",parent->name,std::move(positive),positiveBytes});
    auto minus=std::make_shared<const Piece>(Piece{nextId_+1,parent->id,"Pieza "+std::to_string(nextId_+1)+" (-)",parent->name,std::move(negative),negativeBytes});
    auto after=before;
    const auto position=std::find_if(after.pieces.begin(),after.pieces.end(),[&](const auto& p){return p->id==parent->id;});
    const auto index=static_cast<size_t>(position-after.pieces.begin());
    after.pieces.erase(position);
    after.pieces.insert(after.pieces.begin()+static_cast<std::ptrdiff_t>(index),{plus,minus});
    after.selected=plus->id;
    std::vector<EditState> candidate(states_.begin(),states_.begin()+static_cast<std::ptrdiff_t>(cursor_+1));
    candidate.push_back(std::move(after));
    size_t discarded=0;
    while(candidate.size()>limits_.cuts+1||(candidate.size()>1&&retainedBytes(candidate,original_)>limits_.bytes)) {
        candidate.erase(candidate.begin());++discarded;
    }
    if(retainedBytes(candidate,original_)>limits_.bytes)
        throw std::runtime_error("El nuevo estado no cabe en el presupuesto del historial; se conserva la sesion anterior.");
    states_.swap(candidate);cursor_=states_.size()-1;nextId_+=2;
    return {plus->id,minus->id,discarded};
}
bool EditHistory::undo() {if(!cursor_)return false;--cursor_;return true;}
bool EditHistory::redo() {if(!redoCount())return false;++cursor_;return true;}
}
