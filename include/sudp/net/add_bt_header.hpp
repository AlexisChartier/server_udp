#pragma once
#include <vector>
#include <string>
#include <cstdint>

namespace sudp::net
{

/**
 *  Ajoute – si besoin – l’entête textuel d’un fichier “.bt”
 *  attendu par `octomap::AbstractOcTree::readBinary(std::istream&)`.
 *
 *  Format minimal :
 *      # Octomap OcTree binary file\n
 *      id <TreeType>\n
 *      data\n
 */
inline std::vector<uint8_t>
add_bt_header_if_needed(const std::vector<uint8_t>& blob,
                        const std::string& tree_type = "ColorOcTree")
{
    static const std::string magic = "# Octomap OcTree binary file";

    /*  Déjà présent ? On ne touche à rien. */
    if (blob.size() >= magic.size() &&
        std::equal(magic.begin(), magic.end(), blob.begin()))
        return blob;

    /*  Sinon on pré-fixe le header minimal.                         */
    std::string header = magic + '\n'          // ligne 1
                       + "id " + tree_type + '\n'   // ligne 2
                       + "data\n";                  // ligne 3

    std::vector<uint8_t> out;
    out.reserve(header.size() + blob.size());
    out.insert(out.end(), header.begin(), header.end());
    out.insert(out.end(), blob.begin(),   blob.end());
    return out;
}

} // namespace sudp::net
