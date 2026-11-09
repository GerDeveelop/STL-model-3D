#include "PaintCodec.h"
#include "StlReader.h"
#include <cmath>
#include <functional>
#include <numeric>
#include <stdexcept>
namespace stlcut {
std::vector<PaintLeaf> decodePaint(std::string_view code,std::stop_token stop) {
    if(code.size()>262144)throw std::runtime_error("Pintura: codigo supera 262144 caracteres.");
    std::vector<PaintLeaf> leaves;
    size_t position=code.size();
    auto nibble=[&]()->unsigned {
        if(stop.stop_requested())throw ImportCancelled{};
        if(!position)throw std::runtime_error("Pintura: arbol truncado.");
        const auto c=code[--position];
        if(c>='0'&&c<='9')return c-'0';
        if(c>='a'&&c<='f')return c-'a'+10;
        if(c>='A'&&c<='F')return c-'A'+10;
        throw std::runtime_error("Pintura: se esperaba hexadecimal.");
    };
    auto midpoint=[](Vec3 a,Vec3 b) {return Vec3{std::midpoint(a.x,b.x),std::midpoint(a.y,b.y),std::midpoint(a.z,b.z)};};
    std::function<void(const Triangle&,unsigned)> visit;
    visit=[&](const Triangle& triangle,unsigned depth) {
        if(depth>20)throw std::runtime_error("Pintura: profundidad supera 20.");
        const auto value=nibble();const auto split=value%4;
        if(!split) {
            unsigned filament=value/4;
            if(filament==3) {
                unsigned extra;
                do {extra=nibble();filament+=extra;if(filament>4096)throw std::runtime_error("Pintura: filamento supera 4096.");}while(extra==15);
            }
            if(leaves.size()>=65536)throw std::runtime_error("Pintura: mas de 65536 regiones en una cara.");
            leaves.push_back({triangle,static_cast<std::uint16_t>(filament)});return;
        }
        const auto side=value/4;
        if(side>2||(split==3&&side!=0))throw std::runtime_error("Pintura: subdivision no valida.");
        const auto a=triangle.vertices[side],b=triangle.vertices[(side+1)%3],c=triangle.vertices[(side+2)%3];
        const auto ab=midpoint(a,b),bc=midpoint(b,c),ca=midpoint(c,a);
        std::vector<Triangle> children;
        if(split==1)children={Triangle{{a,b,bc}},Triangle{{bc,c,a}}};
        else if(split==2)children={Triangle{{a,ab,ca}},Triangle{{ab,b,ca}},Triangle{{b,c,ca}}};
        else children={Triangle{{a,ab,ca}},Triangle{{ab,b,bc}},Triangle{{bc,c,ca}},Triangle{{ab,bc,ca}}};
        for(auto child=children.rbegin();child!=children.rend();++child)visit(*child,depth+1);
    };
    const Triangle root{{Vec3{1,0,0},Vec3{0,1,0},Vec3{0,0,1}}};
    if(code.empty())leaves.push_back({root,0});else {visit(root,0);if(position)throw std::runtime_error("Pintura: datos sobrantes tras el arbol.");}
    return leaves;
}
}
