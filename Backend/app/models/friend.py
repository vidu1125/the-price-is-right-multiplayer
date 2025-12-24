from sqlalchemy import (
    Column, Integer, String, Timestamp, ForeignKey, UniqueConstraint
)
from .base import Base

class Friend(Base):
    __tablename__ = "friends"

    id = Column(Integer, primary_key=True)
    requester_id = Column(Integer, ForeignKey("accounts.id"))
    addressee_id = Column(Integer, ForeignKey("accounts.id"))
    status = Column(String(20))

    created_at = Column(Timestamp)
    updated_at = Column(Timestamp)

    __table_args__ = (
        UniqueConstraint("requester_id", "addressee_id"),
    )
