/* DO NOT EDIT or check into source control
 * Generated code for wrapping a ros message Pub/Sub in ecto
 *
 * package : std_msgs
 * msg : Duration
 */
#include <ecto_ros/wrap_sub.hpp>
#include <ecto_ros/wrap_pub.hpp>
#include <ecto_ros/wrap_bag.hpp>


#include <tabletop/TableArray.h>

namespace ecto_tabletop
{
    struct Subscriber_TableArray : ecto_ros::Subscriber<tabletop::TableArray> {};
    struct Publisher_TableArray : ecto_ros::Publisher<tabletop::TableArray> {};
    struct Bagger_TableArray : ecto_ros::Bagger<tabletop::TableArray> {};
}

ECTO_CELL(ecto_tabletop, ecto_tabletop::Subscriber_TableArray,"Subscriber_TableArray", "Subscribes to a std_msgs::Duration.");
ECTO_CELL(ecto_tabletop, ecto_tabletop::Publisher_TableArray,"Publisher_TableArray", "Publishes a std_msgs::Duration.");
ECTO_CELL(ecto_tabletop, ecto_tabletop::Bagger_TableArray,"Bagger_TableArray", "A bagger for messages of a given type. Can enable read/write to ros bags.");
