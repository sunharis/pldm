#pragma once

#include <stdint.h>
#include <unistd.h>

#include <filesystem>
#include <vector>

#include "libpldm/base.h"
#include "oem/ibm/libpldm/file_io.h"

namespace pldm
{

namespace responder
{

namespace oem_ibm
{
/** @brief Register handlers for command from the platform spec
 */
void registerHandlers();
} // namespace oem_ibm

using Response = std::vector<uint8_t>;

namespace dma
{

// The minimum data size of dma transfer in bytes
constexpr uint32_t minSize = 16;

// 16MB - 4096B (16773120 bytes) is the maximum data size of DMA transfer
constexpr size_t maxSize = (16 * 1024 * 1024) - 4096;

namespace fs = std::filesystem;

/**
 * @class DMA
 *
 * Expose API to initiate transfer of data by DMA
 *
 * This class only exposes the public API transferDataHost to transfer data
 * between BMC and host using DMA. This allows for mocking the transferDataHost
 * for unit testing purposes.
 */
class DMA
{
  public:
    /** @brief API to transfer data between BMC and host using DMA
     *
     * @param[in] path     - pathname of the file to transfer data from or to
     * @param[in] offset   - offset in the file
     * @param[in] length   - length of the data to transfer
     * @param[in] address  - DMA address on the host
     * @param[in] upstream - indicates direction of the transfer; true indicates
     *                       transfer to the host
     *
     * @return returns 0 on success, negative errno on failure
     */
    int transferDataHost(const fs::path& path, uint32_t offset, uint32_t length,
                         uint64_t address, bool upstream);
};

/** @brief Transfer the data between BMC and host using DMA.
 *
 *  There is a max size for each DMA operation, transferAll API abstracts this
 *  and the requested length is broken down into multiple DMA operations if the
 *  length exceed max size.
 *
 * @tparam[in] T - DMA interface type
 * @param[in] intf - interface passed to invoke DMA transfer
 * @param[in] command  - PLDM command
 * @param[in] path     - pathname of the file to transfer data from or to
 * @param[in] offset   - offset in the file
 * @param[in] length   - length of the data to transfer
 * @param[in] address  - DMA address on the host
 * @param[in] upstream - indicates direction of the transfer; true indicates
 *                       transfer to the host
 * @param[in] instanceId - Message's instance id
 * @return PLDM response message
 */

template <class DMAInterface>
Response transferAll(DMAInterface* intf, uint8_t command, fs::path& path,
                     uint32_t offset, uint32_t length, uint64_t address,
                     bool upstream, uint8_t instanceId)
{
    uint32_t origLength = length;
    Response response(sizeof(pldm_msg_hdr) + PLDM_RW_FILE_MEM_RESP_BYTES, 0);
    auto responsePtr = reinterpret_cast<pldm_msg*>(response.data());

    while (length > dma::maxSize)
    {
        auto rc = intf->transferDataHost(path, offset, dma::maxSize, address,
                                         upstream);
        if (rc < 0)
        {
            encode_rw_file_memory_resp(instanceId, command, PLDM_ERROR, 0,
                                       responsePtr);
            return response;
        }

        offset += dma::maxSize;
        length -= dma::maxSize;
        address += dma::maxSize;
    }

    auto rc = intf->transferDataHost(path, offset, length, address, upstream);
    if (rc < 0)
    {
        encode_rw_file_memory_resp(instanceId, command, PLDM_ERROR, 0,
                                   responsePtr);
        return response;
    }

    encode_rw_file_memory_resp(instanceId, command, PLDM_SUCCESS, origLength,
                               responsePtr);
    return response;
}

} // namespace dma

/** @brief Handler for readFileIntoMemory command
 *
 *  @param[in] request - pointer to PLDM request payload
 *  @param[in] payloadLength - length of the message
 *
 *  @return PLDM response message
 */
Response readFileIntoMemory(const pldm_msg* request, size_t payloadLength);

/** @brief Handler for writeFileIntoMemory command
 *
 *  @param[in] request - pointer to PLDM request payload
 *  @param[in] payloadLength - length of the message
 *
 *  @return PLDM response message
 */
Response writeFileFromMemory(const pldm_msg* request, size_t payloadLength);

/** @brief Handler for writeFileByTypeFromMemory command
 *
 *  @param[in] request - pointer to PLDM request payload
 *  @param[in] payloadLength - length of the message
 *
 *  @return PLDM response message
 */

Response writeFileByTypeFromMemory(const pldm_msg* request,
                                   size_t payloadLength);

/** @brief Handler for readFileByTypeIntoMemory command
 *
 *  @param[in] request - pointer to PLDM request payload
 *  @param[in] payloadLength - length of the message
 *
 *  @return PLDM response message
 */
Response readFileByTypeIntoMemory(const pldm_msg* request,
                                  size_t payloadLength);

/** @brief Handler for GetFileTable command
 *
 *  @param[in] request - pointer to PLDM request payload
 *  @param[in] payloadLength - length of the message payload
 *
 *  @return PLDM response message
 */
Response getFileTable(const pldm_msg* request, size_t payloadLength);

/** @brief Handler for readFile command
 *
 *  @param[in] request - PLDM request msg
 *  @param[in] payloadLength - length of the message payload
 *
 *  @return PLDM response message
 */
Response readFile(const pldm_msg* request, size_t payloadLength);

/** @brief Handler for writeFile command
 *
 *  @param[in] request - PLDM request msg
 *  @param[in] payloadLength - length of the message payload
 *
 *  @return PLDM response message
 */
Response writeFile(const pldm_msg* request, size_t payloadLength);

/** @brief Handler for readFileByType command
 *
 *  @param[in] request - PLDM request msg
 *  @param[in] payloadLength - length of the message payload
 *
 *  @return PLDM response message
 */
Response readFileByType(const pldm_msg* request, size_t payloadLength);

/** @brief Processes the request for readFileByType command
 *
 *  @param[in] request - pointer to PLDM request payload
 *  @param[in] payloadLength - length of the message
 *  @param[in] fileDir - file path. Applicable when file handle is
 *                  not stored at file table
 *
 */
Response readFileByTypeHandler(const pldm_msg* request, size_t payloadLength,
                               const char* fileDir);
} // namespace responder
} // namespace pldm
